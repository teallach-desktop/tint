// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QString>

enum tint_task_state {
    TASK_ACTIVE = (1 << 0),
    TASK_MINIMIZED = (1 << 1),
};

struct conf {
    /* Colors */
    QString panelBg;
    QString taskActiveBg;
    QString taskInactiveBg;
    QString taskActiveBorder;

    /* Plugins General */
    int pluginMarginLeft;
    int pluginMarginTop;
    int pluginMarginRight;
    int pluginMarginBottom;

    /* Taskbar */
    int taskSpacing;

    /* General */
    QString output;
};

extern conf conf;

void confInit();

QString confGetAppStyle(void);
QString confGetPushButtonStyle(uint32_t state);
