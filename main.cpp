#include "calculator.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Calculator w;
    // 支持 --plain 参数：不应用样式表，用于对比优化前后的界面效果
    if (!(argc > 1 && QLatin1String(argv[1]) == "--plain")) {
        w.applyStyleSheet();
    }
    w.show();

    return app.exec();
}
