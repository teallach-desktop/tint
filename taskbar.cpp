// SPDX-License-Identifier: GPL-2.0-only
#include <QDebug>
#include <QGuiApplication>
#include <QToolButton>
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
    } else if (!strcmp(interface, wl_seat_interface.name)) {
        static_cast<Taskbar *>(data)->addSeat(registry, name, version);
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

Taskbar::Taskbar(QWidget *parent) : QWidget(parent), m_layout(this)
{
    m_layout.setContentsMargins(QMargins(0, 0, 0, 0));
    m_layout.setSpacing(0);

    // Flash up the foreign toplevel interface
    m_display = static_cast<struct wl_display *>(
            QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("wl_display"));
    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registry_listener_impl, this);
}

Taskbar::~Taskbar()
{
    if (m_foreignToplevelManager) {
        qDebug() << "zwlr_foreign_toplevel_manager_v1_destroy()";
        zwlr_foreign_toplevel_manager_v1_destroy(m_foreignToplevelManager);
        m_foreignToplevelManager = nullptr;
    }
    wl_seat_destroy(m_seat);
    wl_registry_destroy(m_registry);
}

int Taskbar::numTasks(void)
{
    int i = 0;
    foreach (QToolButton *button, m_layout.parentWidget()->findChildren<QToolButton *>()) {
        ++i;
    }
    return i;
}

void Taskbar::addTask(struct zwlr_foreign_toplevel_handle_v1 *handle)
{
    // Insert it after the last task, but before the stretch object
    int index = numTasks();
    m_layout.insertWidget(index, new Task(this, handle, m_seat), 0, Qt::AlignLeft);
    if (!index) {
        m_layout.addStretch();
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

void Taskbar::addSeat(struct wl_registry *registry, uint32_t name, uint32_t version)
{
    version = std::min<uint32_t>(version, wl_seat_interface.version);
    m_seat = static_cast<struct wl_seat *>(
            wl_registry_bind(registry, name, &wl_seat_interface, version));
    if (!m_seat) {
        qDebug() << "no wl_seat";
    }
}
