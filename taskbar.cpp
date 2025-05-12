// SPDX-License-Identifier: GPL-2.0-only
#include <QDebug>
#include <QDirIterator>
#include <QGraphicsItem>
#include <QGuiApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
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
#include "item-type.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"

class Task : public QGraphicsItem
{
public:
    Task(QWidget *parent, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_seat *seat);
    ~Task();

    enum { Type = UserType + PANEL_TYPE_TASK };
    int type() const override { return Type; }

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) Q_DECL_OVERRIDE;

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;

private:
    QString getIcon(const char *app_id);
    struct wl_seat *m_seat;
    struct zwlr_foreign_toplevel_handle_v1 *m_handle;
    uint32_t m_state;
    Taskbar *m_taskbar;
    std::string m_app_id;
    bool m_hover;
};

Task::Task(QWidget *parent, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_seat *seat)
    : m_state{ 0 }
{
    m_handle = handle;
    m_seat = seat;
    m_taskbar = static_cast<Taskbar *>(parent);
    m_hover = false;

    setAcceptHoverEvents(true);

    static const zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_impl = {
        .title =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *title) {
                    // static_cast<Task *>(data)->setText(QString(title).replace("&", "&&"));
                },
        .app_id =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *app_id) {
                    auto self = static_cast<Task *>(data);
                    self->m_app_id = app_id;
                    // QIcon icon = QIcon::fromTheme(self->getIcon(app_id));
                    // if (icon.isNull()) {
                    //     qDebug() << "no icon for " << app_id;
                    //  }
                    // self->update();
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
                },
        .done =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle) {
                    // no-op
                },
        .closed =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle) {
                    auto self = static_cast<Task *>(data);
                    auto taskbar = self->m_taskbar;
                    zwlr_foreign_toplevel_handle_v1_destroy(self->m_handle);
                    self->m_handle = nullptr;
                    delete (self);
                    taskbar->updateTasks();
                },
        .parent =
                [](void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                   zwlr_foreign_toplevel_handle_v1 *parent) {
                    // no-op
                },
    };

    zwlr_foreign_toplevel_handle_v1_add_listener(m_handle, &toplevel_handle_impl, this);
}

Task::~Task()
{
    if (m_handle) {
        zwlr_foreign_toplevel_handle_v1_destroy(m_handle);
    }
}

#define PANEL_HEIGHT 32
#define ITEM_HEIGHT 30.0
#define ITEM_WIDTH 80.0
#define ITEM_MARGIN_X 2
#define ITEM_MARGIN_Y 2
#define ITEM_SPACING 2

QRectF Task::boundingRect() const
{
    return QRectF(0.5 + ITEM_MARGIN_X, 0.5 + ITEM_MARGIN_Y, ITEM_WIDTH - 1.0 - 2.0 * ITEM_MARGIN_X,
                  ITEM_HEIGHT - 1.0 - 2.0 * ITEM_MARGIN_Y);
}

void Task::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Panel border
    QPen pen(QColor("#8f8f91"));
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1.0);

    if (m_state & TASK_ACTIVE) {
        painter->setPen(pen);
        painter->setBrush(conf.backgrounds.at(conf.task_active_background_id)->background_color);
    } else {
        painter->setPen(m_hover ? pen : Qt::NoPen);
        painter->setBrush(conf.backgrounds.at(conf.task_background_id)->background_color);
    }
    QPainterPath path;
    path.addRoundedRect(boundingRect(), 4, 4);
    painter->drawPath(path);

    // Text
    QFont font;
    // font.setFamily(font.defaultFamily());
    font.setFamily("Sans");
    font.setPointSize(9);
    painter->setFont(font);
    painter->setPen(QColor("#000000"));
    QRectF rect = boundingRect().adjusted(3, 0, -6, 0);
    QFontMetrics metrics(font);
    QString editedText =
            metrics.elidedText(QString::fromStdString(m_app_id), Qt::ElideRight, rect.width());
    painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, editedText);

    // TODO: Not right place - so just temporary hack
    update();
}

void Task::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    /* TODO: just temporarily to get ASAN output */
    QCoreApplication::quit();
}

void Task::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Traditional minimize-raise action
    if (event->button() == Qt::LeftButton) {
        if (m_state & TASK_MINIMIZED) {
            zwlr_foreign_toplevel_handle_v1_unset_minimized(m_handle);
        } else if (m_state & TASK_ACTIVE) {
            zwlr_foreign_toplevel_handle_v1_set_minimized(m_handle);
        } else {
            zwlr_foreign_toplevel_handle_v1_activate(m_handle, m_seat);
        }
    }

    update();
    QGraphicsItem::mousePressEvent(event);
}

void Task::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mouseReleaseEvent(event);
}

void Task::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    m_hover = true;
    update();
}

void Task::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    m_hover = false;
    update();
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

Taskbar::Taskbar(QWidget *parent, QGraphicsScene *scene, QGraphicsView *view)
    : QWidget(parent), m_scene{ scene }
{
    // Flash up the foreign toplevel interface
    m_display = static_cast<struct wl_display *>(
            QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("wl_display"));
    m_registry = wl_display_get_registry(m_display);

    static const wl_registry_listener registry_listener_impl = {
        .global =
                [](void *data, wl_registry *registry, uint32_t name, const char *interface,
                   uint32_t version) {
                    auto self = static_cast<Taskbar *>(data);
                    if (!strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name)) {
                        self->addForeignToplevelManager(registry, name, version);
                    } else if (!strcmp(interface, wl_seat_interface.name)) {
                        self->addSeat(registry, name, version);
                    }
                },
        .global_remove =
                [](void *data, wl_registry *registry, uint32_t name) {
                    /* no-op */
                }
    };

    wl_registry_add_listener(m_registry, &registry_listener_impl, this);
}

Taskbar::~Taskbar()
{
    if (m_foreignToplevelManager) {
        zwlr_foreign_toplevel_manager_v1_destroy(m_foreignToplevelManager);
        m_foreignToplevelManager = nullptr;
    }
    wl_seat_destroy(m_seat);
    wl_registry_destroy(m_registry);
}

void Taskbar::addTask(struct zwlr_foreign_toplevel_handle_v1 *handle)
{
    m_scene->addItem(new Task(this, handle, m_seat));
    updateTasks();
}

void Taskbar::updateTasks(void)
{
    int i = 0;
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (Task *p = qgraphicsitem_cast<Task *>(item)) {
            int margin = (PANEL_HEIGHT - ITEM_HEIGHT) / 2;
            int y = margin;
            int x = margin + i * (ITEM_WIDTH + ITEM_SPACING);
            p->setPos(x, y);
            i++;
        }
    }
}

void Taskbar::addForeignToplevelManager(struct wl_registry *registry, uint32_t name,
                                        uint32_t version)
{
    m_foreignToplevelManager = static_cast<struct zwlr_foreign_toplevel_manager_v1 *>(
            wl_registry_bind(registry, name, &zwlr_foreign_toplevel_manager_v1_interface, version));
    if (!m_foreignToplevelManager) {
        qDebug() << "foreign-toplevel-management protocol not supported by compositor";
        return;
    }

    static const zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_impl = {
        .toplevel =
                [](void *data, zwlr_foreign_toplevel_manager_v1 *manager,
                   zwlr_foreign_toplevel_handle_v1 *handle) {
                    static_cast<Taskbar *>(data)->addTask(handle);
                },
        .finished =
                [](void *data, zwlr_foreign_toplevel_manager_v1 *manager) {
                    /* no-op */
                },
    };

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
