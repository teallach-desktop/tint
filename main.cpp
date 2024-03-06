// SPDX-License-Identifier: GPL-2.0-only
#include <QApplication>
#include "panel.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Panel panel;
    panel.show();
    return app.exec();
}
