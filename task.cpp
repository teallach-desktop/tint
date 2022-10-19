// SPDX-License-Identifier: GPL-2.0-only
#include "panel.h"
#include "taskbar.h"
#include "task.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

static void handle_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                         const char *title)
{
    // nop
}

static void handle_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                          const char *app_id)
{
    static_cast<Task *>(data)->handle_app_id(app_id);
}

static void handle_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                struct wl_output *output)
{
    // nop
}

static void handle_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                struct wl_output *output)
{
    // nop
}

static void handle_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                         struct wl_array *state)
{
    // nop
}

static void handle_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle)
{
    // nop
}

static void handle_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                          struct zwlr_foreign_toplevel_handle_v1 *parent)
{
    // nop
}

static void handle_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle)
{
    static_cast<Task *>(data)->handle_closed();
}

static const struct zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_impl = {
    .title = handle_title,
    .app_id = handle_app_id,
    .output_enter = handle_output_enter,
    .output_leave = handle_output_leave,
    .state = handle_state,
    .done = handle_done,
    .closed = handle_closed,
    .parent = handle_parent,
};

Task::Task(Panel *panel, Taskbar *taskbar, struct zwlr_foreign_toplevel_handle_v1 *handle)
    : m_panel{ panel }, m_taskbar{ taskbar }, m_handle{ handle }
{
    zwlr_foreign_toplevel_handle_v1_add_listener(m_handle, &toplevel_handle_impl, this);
    m_panel->update();
}

Task::~Task() { }

void Task::handle_app_id(const char *app_id)
{
    m_app_id = app_id;
    m_panel->update();
}

void Task::handle_closed(void)
{
    zwlr_foreign_toplevel_handle_v1_destroy(m_handle);
    m_handle = nullptr;
    m_taskbar->removeTask(this);
}
