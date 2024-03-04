// SPDX-License-Identifier: GPL-2.0-only
#include <QDebug>
#include <QGuiApplication>
#include <QTextStream>
#include <poll.h>
#include <pthread.h>
#include <wayland-client.h>
#include <qpa/qplatformnativeinterface.h>
#include "panel.h"
#include "task.h"
#include "taskbar.h"
#include "wayland-wlr-foreign-toplevel-management-client-protocol.h"

static void handle_wl_registry_global(void *data, struct wl_registry *registry, uint32_t name,
                                      const char *interface, uint32_t version)
{
    if (!strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name)) {
        static_cast<Taskbar *>(data)->addForeignToplevelManager(registry, name, version);
    }
}

static void handle_wl_registry_global_remove(void *data, struct wl_registry *registry,
                                             uint32_t name)
{
    // nop
}

static const struct wl_registry_listener registry_listener_impl = {
    .global = handle_wl_registry_global,
    .global_remove = handle_wl_registry_global_remove,
};

Taskbar::Taskbar(Panel *panel) : m_panel{ panel }
{
    m_panel->addPlugin(this);

    m_display = static_cast<struct wl_display *>(QGuiApplication::platformNativeInterface()
                        ->nativeResourceForIntegration("wl_display"));
    struct wl_registry *registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(registry, &registry_listener_impl, this);
}

Taskbar::~Taskbar() { }

void Taskbar::addTask(struct zwlr_foreign_toplevel_handle_v1 *handle)
{
    m_tasks.push_back(std::make_unique<Task>(this->panel(), this, handle));
}

void Taskbar::removeTask(Task *task)
{
    int i = 0;
    for (auto &t : m_tasks) {
        if (t.get() == task) {
            m_tasks.erase(m_tasks.begin() + i);
            break;
        }
        ++i;
    }
    m_panel->update();
}

void Taskbar::updateTaskPositions(void)
{
    const int margin = 10;
    int x = margin;
    for (auto &task : m_tasks) {
        task->moveTo(x, 0);
        x += task->width() + margin;
    }
}

static void handle_toplevel(void *data, struct zwlr_foreign_toplevel_manager_v1 *manager,
                            struct zwlr_foreign_toplevel_handle_v1 *handle)
{
    static_cast<Taskbar *>(data)->addTask(handle);
}

static void handle_finished(void *data, struct zwlr_foreign_toplevel_manager_v1 *manager)
{
    // nop
}

static const struct zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_impl = {
    .toplevel = handle_toplevel,
    .finished = handle_finished,
};

void Taskbar::addForeignToplevelManager(struct wl_registry *registry, uint32_t name,
                                        uint32_t version)
{
    m_foreignToplevelManager = static_cast<struct zwlr_foreign_toplevel_manager_v1 *>(
            wl_registry_bind(registry, name, &zwlr_foreign_toplevel_manager_v1_interface, version));
    if (!m_foreignToplevelManager) {
        qDebug() << "foreign-toplevel-management protocol not supported by compositor";
        return;
    }
    zwlr_foreign_toplevel_manager_v1_add_listener(m_foreignToplevelManager, &toplevel_manager_impl,
                                                  this);
}
