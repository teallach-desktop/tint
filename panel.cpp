// SPDX-License-Identifier: GPL-2.0-only
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QMetaEnum>
#include <LayerShellQt/shell.h>
#include <LayerShellQt/window.h>
#include <QtWaylandClient/private/qwayland-xdg-shell.h>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QStackedLayout>
#include "conf.h"
#include "item-type.h"
#include "panel.h"
#include "taskbar.h"

class BackgroundItem : public QGraphicsItem
{
public:
    BackgroundItem(int width, int height);
    ~BackgroundItem();
    enum { Type = UserType + PANEL_TYPE_BACKGROUND };
    int type() const override { return Type; }
    QRectF boundingRect() const Q_DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) Q_DECL_OVERRIDE;

private:
    int m_width;
    int m_height;
};

BackgroundItem::BackgroundItem(int width, int height)
{
    m_width = width;
    m_height = height;
}

BackgroundItem::~BackgroundItem() { }

QRectF BackgroundItem::boundingRect() const
{
    return QRectF(0.5, 0.5, m_width - 1.0, m_height - 1.0);
}

void BackgroundItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    // Panel background
    painter->setBrush(QColor("#cecece"));
    painter->setPen(QColor("#000000"));
    painter->drawRect(boundingRect());

    // Panel border
    QPen pen(QColor("#bbbbbb"));
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1.0);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(boundingRect());
}

class View : public QGraphicsView
{
public:
    View(QRect screenGeometry, QWidget *parent = 0);
    ~View();

    void updateView();

private:
    QWidget *m_parent;
    QGraphicsScene m_scene;
};

View::View(QRect screenGeometry, QWidget *parent) : QGraphicsView(parent)
{
    m_parent = parent;
    setScene(&m_scene);

    qDebug() << "screenGeometry=" << screenGeometry;

    int width = screenGeometry.width();
    int height = 32;

    m_scene.setSceneRect(0, 0, width, height);
    m_scene.setItemIndexMethod(QGraphicsScene::NoIndex);

    setRenderHint(QPainter::Antialiasing);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setStyleSheet("background-color: transparent;");
    setFrameStyle(QFrame::NoFrame);

    BackgroundItem *p = new BackgroundItem(width, height);
    m_scene.addItem(p);
    p->setPos(0, 0);

    Taskbar *taskbar = new Taskbar(parent, &m_scene, this);
    (void)taskbar;

    // Set the icon positions
    //    int i = 0;
    //    foreach (QGraphicsItem *item, items()) {
    //        if (Item *p = qgraphicsitem_cast<Item *>(item)) {
    //            int y = 0;
    //            int x = i * 90;
    //            p->setPos(x, y);
    //            i++;
    //        }
    //    }
}

View::~View() { }

Panel::Panel(QWidget *parent) : QMainWindow(parent)
{
    LayerShellQt::Shell::useLayerShell();
    this->winId();
    QWindow *window = this->windowHandle();
    LayerShellQt::Window *layerShell = LayerShellQt::Window::get(window);

    // Panel margins
    layerShell->setMargins({ 0, 0, 0, 0 });

    // Panel layer
    const auto layer = QMetaEnum::fromType<LayerShellQt::Window::Layer>();
    layerShell->setLayer(LayerShellQt::Window::Layer(layer.keyToValue("LayerTop")));

    // Panel position
    const QString panelAnchors = "AnchorBottom|AnchorLeft|AnchorRight";
    const auto anchorEnums = QMetaEnum::fromType<LayerShellQt::Window::Anchor>();
    uint32_t anchors = 0;
    const auto stringList = panelAnchors.split(QLatin1Char('|'));
    for (const auto &value : stringList) {
        anchors |= anchorEnums.keyToValue(qPrintable(value));
    }
    layerShell->setAnchors((LayerShellQt::Window::Anchors)anchors);

    const auto interactivity = QMetaEnum::fromType<LayerShellQt::Window::KeyboardInteractivity>();
    layerShell->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivity(
            interactivity.keyToValue("KeyboardInteractivityNone")));

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_AlwaysShowToolTips);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    constexpr int height = 32;
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    QString outputName;
    for (QScreen *s : screen->virtualSiblings()) {
        outputName = s->name();
        qDebug() << "name=" << outputName << "; geometry=" << s->geometry();
        screenGeometry = s->geometry();
        if (outputName == conf.output) {
            break;
        }
    }
    qDebug() << "Using output " << outputName;

    QRect panelGeometry = screenGeometry;
    panelGeometry.setHeight(height);
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);

    QStackedLayout *layout = new QStackedLayout;
    m_centralWidget->setLayout(layout);

    View *view = new View(screenGeometry, m_centralWidget);
    layout->addWidget(view);

    setFixedSize(panelGeometry.size());
    setGeometry(panelGeometry);
    layerShell->setExclusiveZone(height);

    /*
     * Layer shell surfaces are tied to a particular screen once shown.
     * Hide and show to change screens
     */
    hide();
    show();

    resize(screenGeometry.width(), height);
}

Panel::~Panel() { }
