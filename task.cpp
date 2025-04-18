// SPDX-License-Identifier: GPL-2.0-only
#include <QDebug>
#include <QDirIterator>
#include <QIcon>
#include <QMouseEvent>
#include <QString>
#include "conf.h"
#include "panel.h"
#include "taskbar.h"
#include "task.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"

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
    static_cast<Task *>(data)->handle_state(state);
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

Task::Task(QWidget *parent, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_seat *seat)
    : QToolButton(parent), m_state{0}
{
    m_handle = handle;
    m_seat = seat;
    zwlr_foreign_toplevel_handle_v1_add_listener(m_handle, &toplevel_handle_impl, this);
    this->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    this->setIconSize(QSize(22, 22));
    //this->setFixedSize(120, 18);
    this->setFixedWidth(120);
}

Task::~Task() { }

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

void Task::handle_app_id(const char *app_id)
{
    m_app_id = app_id;
    setText(app_id);
    setToolTip(app_id);
    QIcon icon = QIcon::fromTheme(getIcon(app_id));
    if (icon.isNull()) {
        qDebug() << "no icon for " << app_id;
    }
    setIcon(icon);
}

void Task::handle_state(struct wl_array *state)
{
    m_state = 0;
    for (size_t i = 0; i < state->size / sizeof(uint32_t); ++i) {
        uint32_t elm = static_cast<uint32_t *>(state->data)[i];
        switch (elm) {
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED:
            m_state |= TASK_ACTIVE;
            break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED:
            m_state |= TASK_MINIMIZED;
            break;
        default:
            break;
        }
    }
    this->setStyleSheet(confGetPushButtonStyle(m_state));
}

void Task::handle_closed(void)
{
    zwlr_foreign_toplevel_handle_v1_destroy(m_handle);
    m_handle = nullptr;
    delete (this);
}
