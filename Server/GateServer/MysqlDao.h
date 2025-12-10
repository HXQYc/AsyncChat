#pragma once
#include "const.h"
#include "Singleton.h"
#include <thread>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <mysqlx/xdevapi.h>

using namespace mysqlx;

// SQL 连接包装类，使用 mysqlx Session
class SqlConnection {
public:
    SqlConnection(Session* sess, int64_t lasttime) :_sess(sess), _last_oper_time(lasttime) {}
    std::unique_ptr<Session> _sess;
    int64_t _last_oper_time;
};

// MySQL 连接池类
class MySqlPool {
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false), _fail_count(0) {
        try {
            // 构建连接字符串
            std::string connectionString = "mysqlx://" + user_ + ":" + pass_ + "@" + url_ + "/" + schema_;

            for (int i = 0; i < poolSize_; ++i) {
                Session* sess = new Session(connectionString);
                // 获取当前时间戳
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                // 将时间戳转换为秒
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
                pool_.push(std::make_unique<SqlConnection>(sess, timestamp));
            }

            _check_thread = std::thread([this]() {
                while (!b_stop_) {
                    checkConnectionPro();
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                }
                });

            _check_thread.detach();
        }
        catch (Error& e) {
            // 处理异常
            std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
        }
    }

    void checkConnectionPro() {
        // 1)先读取目标处理数量
        size_t targetCount;
        {
            std::lock_guard<std::mutex> guard(mutex_);
            targetCount = pool_.size();
        }

        //2 当前已经处理的连接
        size_t processed = 0;

        //3 时间戳
        auto now = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

        while (processed < targetCount) {
            std::unique_ptr<SqlConnection> con;
            {
                std::lock_guard<std::mutex> guard(mutex_);
                if (pool_.empty()) {
                    break;
                }
                con = std::move(pool_.front());
                pool_.pop();
            }

            bool healthy = true;
            //检查连接健康/重连逻辑
            if (timestamp - con->_last_oper_time >= 5) {
                try {
                    con->_sess->sql("SELECT 1").execute();
                    con->_last_oper_time = timestamp;
                }
                catch (Error& e) {
                    std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                    healthy = false;
                    _fail_count++;
                }

            }

            if (healthy)
            {
                std::lock_guard<std::mutex> guard(mutex_);
                pool_.push(std::move(con));
                cond_.notify_one();
            }

            ++processed;
        }

        while (_fail_count > 0) {
            auto b_res = reconnect(timestamp);
            if (b_res) {
                _fail_count--;
            }
            else {
                break;
            }
        }
    }

    bool reconnect(long long timestamp) {
        try {
            std::string connectionString = "mysqlx://" + user_ + ":" + pass_ + "@" + url_ + "/" + schema_;
            Session* sess = new Session(connectionString);

            auto newCon = std::make_unique<SqlConnection>(sess, timestamp);
            {
                std::lock_guard<std::mutex> guard(mutex_);
                pool_.push(std::move(newCon));
            }

            std::cout << "mysql connection reconnect success" << std::endl;
            return true;

        }
        catch (Error& e) {
            std::cout << "Reconnect failed, error is " << e.what() << std::endl;
            return false;
        }
    }

    std::unique_ptr<SqlConnection> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !pool_.empty(); });
        if (b_stop_) {
            return nullptr;
        }
        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
        pool_.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        // 更新最后操作时间
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
        con->_last_oper_time = timestamp;
        pool_.push(std::move(con));
        cond_.notify_one();
    }

    const std::string& getSchema() const {
        return schema_;
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }

    ~MySqlPool() {
        b_stop_ = true;
        cond_.notify_all();
        if (_check_thread.joinable()) {
            _check_thread.join();
        }
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }

private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
    std::thread _check_thread;
    std::atomic<int> _fail_count;
};

struct UserInfo {
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
};

class MysqlDao
{
public:
    MysqlDao();
    ~MysqlDao();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& newpwd);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
    bool TestProcedure(const std::string& email, int& uid, std::string& name);
private:
    std::unique_ptr<MySqlPool> pool_;
};
