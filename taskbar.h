// SPDX-License-Identifier: GPL-2.0-only
#pragma once

class Task;

class Taskbar : public QWidget
{
public:
    Taskbar(QWidget *parent);
    ~Taskbar();
    int numTasks(void);
    void addTask(struct zwlr_foreign_toplevel_handle_v1 *);

    void addForeignToplevelManager(struct wl_registry *, uint32_t name, uint32_t version);
    void addSeat(struct wl_registry *registry, uint32_t name, uint32_t version);

private:
    struct wl_display *m_display;
    QHBoxLayout m_layout;
    struct zwlr_foreign_toplevel_manager_v1 *m_foreignToplevelManager;
    struct wl_seat *m_seat;
};
