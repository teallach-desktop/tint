// SPDX-License-Identifier: GPL-2.0-only
#include <QTextStream>
#include "conf.h"

struct conf conf = { 0 };

void confInit(void)
{
    conf.panelBg = "cecece";
    conf.taskActiveBg = "dadbde";
    conf.taskInactiveBg = "cecece";
    conf.taskActiveBorder = "8f8f91";

    conf.pluginMarginLeft = 2;
    conf.pluginMarginTop = 2;
    conf.pluginMarginRight = 2;
    conf.pluginMarginBottom = 2;

    conf.taskSpacing = 2;
}

QString confGetAppStyle(void)
{
    QString ret;

    QTextStream(&ret) << "QMainWindow { background-color: #" << conf.panelBg << ";}";
    QTextStream(&ret) << "QHBoxLayout { padding: 5px; }";

    return ret;
}

QString confGetPushButtonStyle(uint32_t state)
{
    QString bgCol = (state & TASK_ACTIVE) ? conf.taskActiveBg : conf.taskInactiveBg;
    QString borderCol = (state & TASK_ACTIVE) ? conf.taskActiveBorder : bgCol;

    QString ret;

    QTextStream(&ret) << "QToolButton {";
    QTextStream(&ret) << "    border: 1px solid #" << borderCol << ";";
    QTextStream(&ret) << "    border-radius: 4px;";
    QTextStream(&ret) << "    background-color: #" << bgCol << ";";
    QTextStream(&ret) << "    margin: 0px 0px 0px 0px;";
    QTextStream(&ret) << "}";
    QTextStream(&ret) << "QToolButton:hover {";
    QTextStream(&ret) << "    background-color: #eeeeee;";
    QTextStream(&ret) << "}";

    return ret;
}
