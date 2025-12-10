#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <iostream>

MysqlDao::MysqlDao()
{
    auto& cfg = ConfigMgr::Inst();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
    if (pool_) {
        pool_->Close();
    }
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    auto con = pool_->getConnection();
    if (con == nullptr || con->_sess == nullptr) {
        return -1;
    }

    try {
        Session& sess = *(con->_sess);

        // 准备调用存储过程
        SqlStatement stmt = sess.sql("CALL reg_user(?,?,?,@result)");
        stmt.bind(name, email, pwd);

        // 执行存储过程
        stmt.execute();

        // 获取存储过程的返回值
        RowResult res = sess.sql("SELECT @result AS result").execute();
        Row row = res.fetchOne();
        if (row) {
            int result = row[0].get<int>();
            std::cout << "Result: " << result << std::endl;
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return -1;
    }
    catch (Error& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

int MysqlDao::RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd)
{
    auto con = pool_->getConnection();
    if (con == nullptr || con->_sess == nullptr) {
        return -1;
    }

    try {
        Session& sess = *(con->_sess);

        // 开始事务
        sess.startTransaction();

        // 执行第一个数据库操作：检查email是否已存在
        RowResult res_email = sess.sql("SELECT 1 FROM user WHERE email = ?").bind(email).execute();
        if (res_email.count() > 0) {
            sess.rollback();
            std::cout << "email " << email << " exist" << std::endl;
            pool_->returnConnection(std::move(con));
            return 0;
        }

        // 准备查询用户名是否重复
        RowResult res_name = sess.sql("SELECT 1 FROM user WHERE name = ?").bind(name).execute();
        if (res_name.count() > 0) {
            sess.rollback();
            std::cout << "name " << name << " exist" << std::endl;
            pool_->returnConnection(std::move(con));
            return 0;
        }

        // 准备更新用户id
        sess.sql("UPDATE user_id SET id = id + 1").execute();

        // 获取更新后的 id 值
        RowResult res_uid = sess.sql("SELECT id FROM user_id").execute();
        Row row_uid = res_uid.fetchOne();
        int newId = 0;
        // 处理结果集
        if (row_uid) {
            newId = row_uid[0].get<int>();
        }
        else {
            std::cout << "select id from user_id failed" << std::endl;
            sess.rollback();
            pool_->returnConnection(std::move(con));
            return -1;
        }

        // 插入user信息
        sess.sql("INSERT INTO user (uid, name, email, pwd) VALUES (?, ?, ?, ?)")
            .bind(newId, name, email, pwd)
            .execute();

        // 提交事务
        sess.commit();
        std::cout << "newuser insert into user success" << std::endl;
        pool_->returnConnection(std::move(con));
        return newId;
    }
    catch (Error& e) {
        // 发生异常，回滚事务
        if (con && con->_sess) {
            try {
                con->_sess->rollback();
            }
            catch (...) {}
        }
        std::cerr << "Error: " << e.what() << std::endl;
        pool_->returnConnection(std::move(con));
        return -1;
    }
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
    auto con = pool_->getConnection();
    if (con == nullptr || con->_sess == nullptr) {
        return false;
    }

    try {
        Session& sess = *(con->_sess);

        // 准备查询语句
        RowResult res = sess.sql("SELECT email FROM user WHERE name = ?").bind(name).execute();

        // 处理结果集
        Row row = res.fetchOne();
        if (row) {
            std::string db_email = row[0].get<std::string>();
            std::cout << "Check Email: " << db_email << std::endl;
            bool result = (email == db_email);
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return false;
    }
    catch (Error& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
    auto con = pool_->getConnection();
    if (con == nullptr || con->_sess == nullptr) {
        return false;
    }

    try {
        Session& sess = *(con->_sess);

        // 准备更新语句
        SqlResult res = sess.sql("UPDATE user SET pwd = ? WHERE name = ?")
            .bind(newpwd, name)
            .execute();

        std::cout << "Updated rows: " << res.getAffectedItemsCount() << std::endl;
        pool_->returnConnection(std::move(con));
        return true;
    }
    catch (Error& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
    auto con = pool_->getConnection();
    if (con == nullptr || con->_sess == nullptr) {
        return false;
    }

    try {
        Session& sess = *(con->_sess);

        // 注意：原代码实现中使用email查询，但头文件接口参数是name
        // 为保持接口不变，这里使用name查询（如果原代码有bug，需要调用方修正）
        // 使用明确的列顺序，避免 SELECT * 的不确定性
        RowResult res = sess.sql("SELECT uid, name, email, pwd, nick FROM user WHERE name = ?").bind(name).execute();

        std::string origin_pwd = "";
        // 处理结果集
        Row row = res.fetchOne();
        if (row) {
            // 使用数字索引访问：uid=0, name=1, email=2, pwd=3
            origin_pwd = row[3].get<std::string>();  // pwd 在第4列（索引3）
            // 打印查询到的密码
            std::cout << "Password: " << origin_pwd << std::endl;

            if (pwd != origin_pwd) {
                pool_->returnConnection(std::move(con));
                return false;
            }
            userInfo.uid = row[0].get<int>();        // uid 在第1列（索引0）
            userInfo.name = row[1].get<std::string>(); // name 在第2列（索引1）
            userInfo.email = row[2].get<std::string>(); // email 在第3列（索引2）
            userInfo.pwd = origin_pwd;
            pool_->returnConnection(std::move(con));
            return true;
        }
        pool_->returnConnection(std::move(con));
        return false;
    }
    catch (Error& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::TestProcedure(const std::string& email, int& uid, std::string& name) {
    auto con = pool_->getConnection();
    if (con == nullptr || con->_sess == nullptr) {
        return false;
    }

    try {
        Session& sess = *(con->_sess);

        // 准备调用存储过程
        SqlStatement stmt = sess.sql("CALL test_procedure(?,@userId,@userName)");
        stmt.bind(email);

        // 执行存储过程
        stmt.execute();

        // 获取存储过程的输出参数值
        RowResult res_uid = sess.sql("SELECT @userId AS uid").execute();
        Row row_uid = res_uid.fetchOne();
        if (!row_uid) {
            pool_->returnConnection(std::move(con));
            return false;
        }

        uid = row_uid[0].get<int>();
        std::cout << "uid: " << uid << std::endl;

        RowResult res_name = sess.sql("SELECT @userName AS name").execute();
        Row row_name = res_name.fetchOne();
        if (!row_name) {
            pool_->returnConnection(std::move(con));
            return false;
        }

        name = row_name[0].get<std::string>();
        std::cout << "name: " << name << std::endl;
        pool_->returnConnection(std::move(con));
        return true;

    }
    catch (Error& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}