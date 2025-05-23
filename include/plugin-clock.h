// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QGraphicsItem>
#include <QFont>
#include <QString>
#include <QTimer>
#include "item-type.h"

class ClockItem : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    ClockItem(QObject *parent = Q_NULLPTR, int height = 0);
    ~ClockItem();
    enum { Type = UserType + PANEL_TYPE_CLOCK };
    int type() const override { return Type; }
    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QRectF fullDrawingRect();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) Q_DECL_OVERRIDE;

public slots:
    void setTime();

private:
    int m_width;
    int m_height;
    QString m_text;
    QFont m_font;
    QTimer m_timer;
};
