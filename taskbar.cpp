// SPDX-License-Identifier: GPL-2.0-only
#include <QDebug>
#include <QDirIterator>
#include <QGuiApplication>
#include <QIcon>
#include <QMouseEvent>
#include <QString>
#include <QToolButton>
#include <QTextStream>
#include <poll.h>
#include <pthread.h>
#include <wayland-client.h>
#include <qpa/qplatformnativeinterface.h>
#include "panel.h"
#include "conf.h"
#include "taskbar.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"

class Task : public QToolButton
{
public:
    Task(QWidget *parent, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_seat *seat);
    ~Task();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString getIcon(const char *app_id);
    struct wl_seat *m_seat;
    struct zwlr_foreign_toplevel_handle_v1 *m_handle;
    uint32_t m_state;
    std::string m_app_id;

public:
    bool active() const { return m_state & TASK_ACTIVE; }
    bool minimized() const { return m_state & TASK_MINIMIZED; }
};

Task::Task(QWidget *parent, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_seat *seat)
    : QToolButton(parent), m_state{ 0 }
{
    m_handle = handle;
    m_seat = seat;

    static const zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_impl = {
        .title =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *title) {
                    static_cast<Task *>(data)->setText(QString(title).replace("&", "&&"));
                },
        .app_id =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *app_id) {
                    auto self = static_cast<Task *>(data);
                    self->m_app_id = app_id;
                    self->setText(app_id);
                    self->setToolTip(app_id);
                    QIcon icon = QIcon::fromTheme(self->getIcon(app_id));
                    if (icon.isNull()) {
                        qDebug() << "no icon for " << app_id;
                    }
                    self->setIcon(icon);
                },
        .output_enter =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_output *output) {
                    // no-op
                },
        .output_leave =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_output *output) {
                    // no-op
                },
        .state =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_array *state) {
                    auto self = static_cast<Task *>(data);
                    self->m_state = 0;
                    for (size_t i = 0; i < state->size / sizeof(uint32_t); ++i) {
                        uint32_t elm = static_cast<uint32_t *>(state->data)[i];
                        switch (elm) {
                        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED:
                            self->m_state |= TASK_ACTIVE;
                            break;
                        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED:
                            self->m_state |= TASK_MINIMIZED;
                            break;
                        default:
                            break;
                        }
                    }
                    self->setStyleSheet(confGetPushButtonStyle(self->m_state));
                },
        .done =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle) {
                    // no-op
                },
        .closed =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle) {
                    auto self = static_cast<Task *>(data);
                    zwlr_foreign_toplevel_handle_v1_destroy(self->m_handle);
                    self->m_handle = nullptr;
                    delete (self);
                },
        .parent =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                   zwlr_foreign_toplevel_handle_v1 *parent) {
                    // no-op
                },
    };

    zwlr_foreign_toplevel_handle_v1_add_listener(m_handle, &toplevel_handle_impl, this);
    this->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    this->setIconSize(QSize(22, 22));
    // this->setFixedSize(120, 18);
    this->setFixedWidth(120);
}

Task::~Task()
{
    zwlr_foreign_toplevel_handle_v1_destroy(m_handle);
}

void Task::mousePressEvent(QMouseEvent *event)
{
    qDebug() << QString::fromStdString(m_app_id);

    // Traditional minimize-raise action
    if (event->button() == Qt::LeftButton) {
        if (minimized()) {
            zwlr_foreign_toplevel_handle_v1_unset_minimized(m_handle);
        } else if (active()) {
            zwlr_foreign_toplevel_handle_v1_set_minimized(m_handle);
        } else {
            zwlr_foreign_toplevel_handle_v1_activate(m_handle, m_seat);
        }
    }
    if (event->button() == Qt::RightButton) {
        QCoreApplication::quit();
    }
}

// Not the prettiest, but just to get a simple prototype going
QString Task::getIcon(const char *app_id)
{
    QDirIterator it(QString("/usr/share/applications"));
    while (it.hasNext()) {
        QString filename = it.next();
        if (!filename.contains(QString::fromStdString(app_id) + ".desktop")) {
            continue;
        }
        QFile f(filename);
        if (!f.open(QIODevice::ReadOnly)) {
            qDebug() << "cannot read file" << f.fileName();
            continue;
        }
        bool inEntry = false;
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.contains("[Desktop Entry]")) {
                inEntry = true;
                continue;
            } else if (line.startsWith("[")) {
                inEntry = false;
                continue;
            }
            if (!inEntry) {
                continue;
            }
            if (line.startsWith("Icon=")) {
                return line.replace("Icon=", "");
            }
        }
        break;
    }
    return nullptr;
}

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
        (void)button;
        ++i;
    }
    return i;
}

void Taskbar::addTask(struct zwlr_foreign_toplevel_handle_v1 *handle)
{
    // Insert it after the last task, but before the stretch object
    int index = numTasks();
    m_layout.insertWidget(index, new Task(this, handle, m_seat), 0, Qt::AlignLeft);
    m_layout.setSpacing(conf.taskSpacing);
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
