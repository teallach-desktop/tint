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
            qWarn() << "cannot read file" << f.fileName();
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
