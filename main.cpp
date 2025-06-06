#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM","windows:darkmode=0");
    QApplication a(argc, argv);
    Widget w;           //创建窗体结构
    w.show();           //展示窗体结构
    return a.exec();    //死循环防止闪退
}
