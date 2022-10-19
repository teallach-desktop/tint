// SPDX-License-Identifier: GPL-2.0-only
#include <QGuiApplication>
#include "panel.h"
#include "taskbar.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    Panel panel;
    Taskbar taskbar(&panel);

    panel.show();
    return app.exec();
}
