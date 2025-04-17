// SPDX-License-Identifier: GPL-2.0-only
#include <QApplication>
#include "conf.h"
#include "panel.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    confInit();
    app.setStyleSheet(confGetAppStyle());

    Panel panel;
    panel.show();
    return app.exec();
}
