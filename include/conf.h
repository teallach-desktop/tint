// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QString>
#include <QColor>
#include <QFont>

enum tint_task_state {
    TASK_ACTIVE = (1 << 0),
    TASK_MINIMIZED = (1 << 1),
};

struct padding {
    int horizontal;
    int vertical;
    int spacing;
};

class Background
{
public:
    Background();
    ~Background();

    int rounded;
    QColor background_color;
    QColor border_color;
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
    int taskbar_background_id;
    struct padding taskbar_padding;

    // Task
    int task_maximum_size;
    QFont task_font;
    QColor task_font_color;
    int task_background_id;
    int task_active_background_id;

    // Clock
    int clock_background_id;
    QFont time1_font;
    QColor clock_font_color;

    /* General (not set by config file) */
    QString output;
    double penWidth;
    int verbosity;

};

extern conf conf;

void confInit(QString filename);
void confSetOutput(QString output);
void confSetVerbosity(int verbosity);
