// SPDX-License-Identifier: GPL-2.0-only
#include <QDebug>
#include <QTextStream>
#include <wayland-client.h>
#include <poll.h>
#include <pthread.h>
#include "panel.h"
#include "taskbar.h"
#include "task.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

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

static void *eventloop(void *arg)
{
    struct wl_display *wl_display = static_cast<struct wl_display *>(arg);
    struct pollfd pollfds[] = { {
            .fd = wl_display_get_fd(wl_display),
            .events = POLLIN,
    } };
    while (true) {
        int ret = 1;
        while (ret > 0) {
            ret = wl_display_dispatch_pending(wl_display);
            wl_display_flush(wl_display);
        }
        if (ret < 0) {
            qDebug() << "wl_display_dispatch_pending()";
            break;
        }
        if (poll(pollfds, 1, 50) < 0) {
            if (errno == EINTR)
                continue;
            qDebug() << "poll()";
            break;
        }
        if ((pollfds[0].revents & POLLIN) && wl_display_dispatch(wl_display) == -1) {
            qDebug() << "wl_display_dispatch()";
            break;
        }
        if ((pollfds[0].revents & POLLOUT) && wl_display_flush(wl_display) == -1) {
            qDebug() << "wl_display_flush()";
            break;
        }
    }
    return NULL;
}

Taskbar::Taskbar(Panel *panel) : m_panel{ panel }
{
    m_panel->addPlugin(this);

    // It's hard to get Qt's wl_display so we just use our own connection in a separate thread
    m_display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(registry, &registry_listener_impl, this);
    pthread_create(&m_eventLoop, NULL, eventloop, m_display);
}

Taskbar::~Taskbar()
{
    pthread_join(m_eventLoop, NULL);
}

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
