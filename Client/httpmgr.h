#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>

// CRTP（奇异递归模板）
class HttpMgr: public QObject,
        public Singleton<HttpMgr>,
        public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT
public:
    virtual ~HttpMgr();                 // 多重继承需要虚析构

    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);

private:
    friend class Singleton<HttpMgr>;    // singleton中getinstance里面new T构造，所以需要声明友元
    HttpMgr();
    QNetworkAccessManager _manager;


private slots:
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);  // 路由分发给对应的业务处理逻辑
signals:
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);   // 网络层的统一出口信号
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);             // 业务处理：注册模块信号
};

#endif // HTTPMGR_H
