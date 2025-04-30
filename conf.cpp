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

