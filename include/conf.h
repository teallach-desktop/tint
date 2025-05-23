// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QString>
#include <QColor>

enum tint_task_state {
    TASK_ACTIVE = (1 << 0),
    TASK_MINIMIZED = (1 << 1),
};

class Background
{
public:
    Background();
    ~Background();

    int rounded;
    QColor background_color;
};

struct conf {
    // Backgrounds
    std::vector<std::unique_ptr<Background>> backgrounds;

    // Panel
    std::string panel_items_left;
    std::string panel_items_right;
    int panel_background_id;
    int panel_height;

    // Taskbar
    int taskbar_padding_horizontal;
    int taskbar_padding_vertical;
    int taskbar_padding_spacing;

    // Task
    int task_maximum_size;
    int task_background_id;
    int task_active_background_id;

    /* General */
    QString output;
    double penWidth;
};

extern conf conf;

void confInit(QString filename);
void confSetOutput(QString output);
