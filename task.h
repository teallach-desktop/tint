// SPDX-License-Identifier: GPL-2.0-only
#pragma once

class Panel;
class Taskbar;

class Task
{
public:
    Task(Panel *panel, Taskbar *taskbar, struct zwlr_foreign_toplevel_handle_v1 *handle);
    ~Task();

    void moveTo(int x, int y);
    void mousePressEvent(QMouseEvent *event);

    // Implementation for zwlr_foreign_toplevel_handle_v1_listener signals
    void handle_app_id(const char *app_id);
    void handle_closed(void);

private:
    Panel *m_panel;
    Taskbar *m_taskbar;
    QRect m_rect;
    struct zwlr_foreign_toplevel_handle_v1 *m_handle;
    std::string m_app_id;

public:
    // Getters
    std::string app_id() const { return m_app_id; }
    int width() const { return m_rect.width(); }
    QRect rect() const { return m_rect; }
};
