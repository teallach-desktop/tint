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
#include "conf.h"
#include "log.h"
#include "item-type.h"
#include "panel.h"
#include "taskbar.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"

class Task : public QGraphicsItem
{
public:
    Task(QGraphicsItem *parent, struct zwlr_foreign_toplevel_handle_v1 *handle,
         struct wl_seat *seat);
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
    struct wl_seat *m_seat;
    struct zwlr_foreign_toplevel_handle_v1 *m_handle;
    uint32_t m_state;
    Taskbar *m_taskbar;
    std::string m_app_id;
    QPixmap m_icon;
    bool m_hover;
};

static QPixmap getIcon(struct sfdo *sfdo, const char *app_id)
{
    int size = 22;
    float scale = 1.0;
    std::string name = load_icon_from_app_id(sfdo, app_id, size, scale);
    QPixmap pixmap = name.empty() ? QPixmap()
                                  : QIcon(QString::fromStdString(name)).pixmap(QSize(size, size));
    return pixmap;
}

Task::Task(QGraphicsItem *parent, struct zwlr_foreign_toplevel_handle_v1 *handle,
           struct wl_seat *seat)
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
                    self->m_icon = getIcon(self->m_taskbar->sfdo(), app_id);
                    self->update();
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
                    self->update();
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

int itemHeight(void)
{
    // Follows panel height
    return conf.panel_height - conf.taskbar_padding.vertical * 2;
}

QRectF Task::boundingRect() const
{
    // TODO: Why is taskWidth() in here??
    return QRectF(0.5 + conf.taskbar_padding.horizontal, 0.5 + conf.taskbar_padding.vertical,
                  m_taskbar->taskWidth() - 1.0 - 2.0 * conf.taskbar_padding.horizontal,
                  itemHeight() - 1.0 - 2.0 * conf.taskbar_padding.vertical);
}

void Task::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen(QColor(conf.backgrounds.at(conf.task_active_background_id)->border_color));
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(conf.penWidth);

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

    // Icon
    if (!m_icon.isNull()) {
        int size = 22;
        int offset = 5;
        QRect target(offset, offset, size, size);
        QRect source(0, 0, size, size);
        painter->drawPixmap(target, m_icon, source);
    }

    // Text
    painter->setFont(conf.task_font);
    painter->setPen(conf.task_font_color);
    QRectF rect = boundingRect().adjusted(3 + 22 + 2 + 2, 0, -6, 0);
    QFontMetrics metrics(conf.task_font);
    QString s = metrics.elidedText(QString::fromStdString(m_app_id), Qt::ElideRight, rect.width());
    painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, s);
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

Taskbar::Taskbar(QGraphicsScene *scene, int height, int width, struct sfdo *sfdo) : m_scene{ scene }
{
    m_height = height;
    m_width = width;
    m_sfdo = sfdo;

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

QRectF Taskbar::boundingRect() const
{
    return QRectF(0, 0, m_width, m_height);
}

QRectF Taskbar::fullDrawingRect()
{
    double halfPenWidth = conf.penWidth / 2.0;
    return boundingRect().adjusted(halfPenWidth, halfPenWidth, -halfPenWidth, -halfPenWidth);
}

void Taskbar::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen(QColor(conf.backgrounds.at(conf.taskbar_background_id)->border_color));
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(conf.penWidth);
    painter->setPen(pen);
    painter->setBrush(conf.backgrounds.at(conf.taskbar_background_id)->background_color);
    painter->drawRect(fullDrawingRect());
}

void Taskbar::addTask(struct zwlr_foreign_toplevel_handle_v1 *handle)
{
    m_scene->addItem(new Task(this, handle, m_seat));
    updateTasks();
}

void Taskbar::updateTasks(void)
{
    int width = taskWidth();
    int i = 0;
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (Task *p = qgraphicsitem_cast<Task *>(item)) {
            int margin = (conf.panel_height - itemHeight()) / 2;
            int y = margin;
            int x = this->x() + margin + i * (width + conf.taskbar_padding.spacing);
            p->setPos(x, y);
            i++;
        }
    }
}

int Taskbar::taskWidth(void)
{
    int nrItems = 0;
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (item->type() == QGraphicsItem::UserType + PANEL_TYPE_TASK) {
            ++nrItems;
        }
    }
    int width = m_scene->width();
    width -= conf.taskbar_padding.horizontal * 2;
    if (nrItems) {
        width -= conf.taskbar_padding.spacing * (nrItems - 1);
    }
    if (nrItems) {
        width /= nrItems;
    }
    if (width > conf.task_maximum_size) {
        width = conf.task_maximum_size;
    }
    return width;
}

void Taskbar::addForeignToplevelManager(struct wl_registry *registry, uint32_t name,
                                        uint32_t version)
{
    m_foreignToplevelManager = static_cast<struct zwlr_foreign_toplevel_manager_v1 *>(
            wl_registry_bind(registry, name, &zwlr_foreign_toplevel_manager_v1_interface, version));
    if (!m_foreignToplevelManager)
        die("foreign-toplevel-management protocol not supported by compositor");

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
    if (!m_seat)
        die("no wl_seat");
}
