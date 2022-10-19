// SPDX-License-Identifier: GPL-2.0-only
#pragma once

class Panel;
class Task;

class Taskbar
{
public:
    Taskbar(Panel *panel);
    ~Taskbar();
    void addTask(struct zwlr_foreign_toplevel_handle_v1 *);
    void removeTask(Task *task);
    void updateTaskPositions(void);
    void addForeignToplevelManager(struct wl_registry *, uint32_t name, uint32_t version);

private:
    Panel *m_panel;
    pthread_t m_eventLoop;
    std::vector<std::unique_ptr<Task>> m_tasks;
    struct zwlr_foreign_toplevel_manager_v1 *m_foreignToplevelManager;
    struct wl_display *m_display;

public:
    // Getters
    Panel *panel() const { return m_panel; }
    Task *task(size_t i) const { return m_tasks.at(i).get(); }
    size_t numTasks() const { return m_tasks.size(); }
};
