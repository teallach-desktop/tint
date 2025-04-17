// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QToolButton>
#include "conf.h"

class Taskbar;

class Task : public QToolButton
{
public:
    Task(QWidget *parent, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_seat *seat);
    ~Task();

    // Foreign toplevel handlers
    void handle_app_id(const char *app_id);
    void handle_state(struct wl_array *state);
    void handle_closed(void);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString getIcon(const char *app_id);
    struct wl_seat *m_seat;
    struct zwlr_foreign_toplevel_handle_v1 *m_handle;
    uint32_t m_state;
    std::string m_app_id;

public:
    uint32_t state() const { return m_state; }
    bool active() const { return m_state & TASK_ACTIVE; }
    bool minimized() const { return m_state & TASK_MINIMIZED; }
};
