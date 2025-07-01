// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QGraphicsView>
#include "item-type.h"

class Taskbar : public QGraphicsItem
{
public:
    Taskbar(QGraphicsScene *scene, int height, int width, struct sfdo *sfdo);
    ~Taskbar();

    enum { Type = UserType + PANEL_TYPE_TASKBAR };
    int type() const override { return Type; }

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QRectF fullDrawingRect();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) Q_DECL_OVERRIDE;

    void addTask(struct zwlr_foreign_toplevel_handle_v1 *);
    void updateTasks(void);
    int taskWidth(void);

private:
    void addForeignToplevelManager(struct wl_registry *, uint32_t name, uint32_t version);
    void addSeat(struct wl_registry *registry, uint32_t name, uint32_t version);
    struct wl_display *m_display;
    struct zwlr_foreign_toplevel_manager_v1 *m_foreignToplevelManager;
    struct wl_registry *m_registry;
    int m_width;
    int m_height;
    QGraphicsScene *m_scene;
    struct sfdo *m_sfdo;

public:
    // Getters
    struct sfdo *sfdo() const { return m_sfdo; }
};
