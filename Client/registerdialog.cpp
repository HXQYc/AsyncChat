#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "global.h"
#include "httpmgr.h"

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);

    // UI设置
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state", "normal");        // 设置属性名："state"、属性值："normal"（正常状态）
    repolish(ui->err_tip);

    // 连接信号
    //
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish,
            this, &RegisterDialog::slot_reg_mod_finish);

    // 初始化处理器映射表
    initHttpHandlers();
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_get_code_clicked()
{
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if (match)
    {
        // 发送http验证码
    }
    else
    {
        showTip(tr("邮箱地址不正确"), false);
    }
}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }

    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"),false);
        return;
    }

    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),false);
        return;
    }


    //调用对应的逻辑,根据id回调。
    _handlers[id](jsonDoc.object());

    return;
}

void RegisterDialog::showTip(QString str, bool b_ok)
{
    if (b_ok)
    {
        ui->err_tip->setProperty("state", "normal");
    }
    else
    {
        ui->err_tip->setProperty("state", "err");
    }

    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

void RegisterDialog::initHttpHandlers()
{
    // 注册获取验证码回包逻辑
    // 映射：获取验证码请求ID_GET_VERIFY_CODE => 处理函数lambda
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送到邮箱，注意查收"), true);
        qDebug()<< "email is " << email ;
    });

//    //注册注册用户回包逻辑
//    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj){
//        int error = jsonObj["error"].toInt();
//        if(error != ErrorCodes::SUCCESS){
//            showTip(tr("参数错误"),false);
//            return;
//        }
//        auto email = jsonObj["email"].toString();
//        showTip(tr("用户注册成功"), true);
//        qDebug()<< "email is " << email ;
//        qDebug()<< "user uuid is " <<  jsonObj["uid"].toString();
//        ChangeTipPage();
//    });
}
