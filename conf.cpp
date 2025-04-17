// SPDX-License-Identifier: GPL-2.0-only
#include "conf.h"

struct conf conf = {0};

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
    return "QMainWindow { background-color: #" + conf.panelBg + ";}"
        "QHBoxLayout { padding: 5px; }";
}

QString confGetPushButtonStyle(uint32_t state)
{
    QString bgCol = (state & TASK_ACTIVE) ? conf.taskActiveBg : conf.taskInactiveBg;
    QString borderCol = (state & TASK_ACTIVE) ? conf.taskActiveBorder: bgCol;

    return "QToolButton {"
        "    border: 1px solid #" + borderCol + ";"
        "    border-radius: 4px;"
        "    background-color: #" + bgCol + ";"
        "    margin: 0px 0px 0px 0px;"
        "}"
        "QToolButton:hover {"
        "    background-color: #eeeeee;"
        "}";
}
