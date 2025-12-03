#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 读取qss文件
    QFile qss(":/style/stylesheet.qss");
    if (qss.open(QFile::ReadOnly))
    {
        qDebug("qss file open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }
    else
    {
        qDebug("qss file open failed");
    }

    MainWindow w;
    w.show();
    return a.exec();
}
