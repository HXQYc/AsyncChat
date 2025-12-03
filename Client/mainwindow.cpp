#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _login_dlg = new LoginDialog(this);
    setCentralWidget(_login_dlg);   // 将登录对话框设置为主窗口的中央部件
    // _login_dlg->show();

    // 创建和注册消息链接
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    _reg_dlg = new RegisterDialog(this);

    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);  // Qt::CustomizeWindowHint：关闭默认的窗口行为
    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);    // FramelessWindowHint：无边框窗口，这种窗口不会自动作为子窗口显示
    _reg_dlg->hide();

}

MainWindow::~MainWindow()
{
    delete ui;

}

void MainWindow::SlotSwitchReg()
{
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
}

