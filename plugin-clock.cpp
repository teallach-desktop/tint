// SPDX-License-Identifier: GPL-2.0-only
#include <QDebug>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QString>
#include <QTimer>
#include <QObject>
#include <QtWidgets>
#include "conf.h"
#include "item-type.h"
#include "plugin-clock.h"

ClockItem::ClockItem(QObject *parent, int height) : QObject(parent)
{
    m_height = height;

    m_text = "MM:MM";
    m_font = QFont("Sans", 10);
    QFontMetrics fm(m_font);
    m_width = fm.horizontalAdvance(m_text) + 3 + 3 + 1;
    m_text = QDateTime::currentDateTime().toString("hh:mm");

    QObject::connect(&m_timer, &QTimer::timeout, this, &ClockItem::setTime);
    m_timer.start(1000);
}

ClockItem::~ClockItem() { }

QRectF ClockItem::boundingRect() const
{
    return QRectF(0, 0, m_width, m_height);
}

QRectF ClockItem::fullDrawingRect()
{
    double halfPenWidth = conf.penWidth / 2.0;
    return boundingRect().adjusted(halfPenWidth, halfPenWidth, -halfPenWidth, -halfPenWidth);
}

void ClockItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen(QColor(conf.backgrounds.at(conf.clock_background_id)->border_color));
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(conf.penWidth);
    painter->setPen(pen);
    painter->setBrush(conf.backgrounds.at(conf.clock_background_id)->background_color);
    painter->drawRect(fullDrawingRect());

    painter->setFont(m_font);
    painter->setPen(QColor("#000000"));
    // TODO: add config padding stuff here
    QRectF rect = fullDrawingRect().adjusted(3, 0, -6, 0);
    QFontMetrics metrics(m_font);
    QString text = metrics.elidedText(m_text, Qt::ElideRight, rect.width());
    painter->drawText(rect, Qt::AlignCenter | Qt::AlignVCenter, text);
}

void ClockItem::setTime()
{
    QString text = QDateTime::currentDateTime().toString("hh:mm");
    if (text == m_text)
        return;
    m_text = text;
    update();
}
