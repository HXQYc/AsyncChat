#pragma once
#include "const.h"
#include "MysqlDao.h"

// 
class MysqlMgr : public Singleton<MysqlMgr>
{
    friend class Singleton<MysqlMgr>;
public:
    // 
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd) {
        return dao_.RegUserTransaction(name, email, pwd);
    }
private:
    MysqlMgr() : dao_() {}
    MysqlDao dao_;
};