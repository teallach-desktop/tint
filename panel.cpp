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
#include <QTimer>
#include <QStackedLayout>
#include "conf.h"
#include "item-type.h"
#include "log.h"
#include "panel.h"
#include "plugin-clock.h"
#include "plugin-taskbar.h"

class BackgroundItem : public QGraphicsItem
{
public:
    BackgroundItem(int width, int height);
    ~BackgroundItem();
    enum { Type = UserType + PANEL_TYPE_BACKGROUND };
    int type() const override { return Type; }
    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QRectF fullDrawingRect();
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
    return QRectF(0, 0, m_width, m_height);
}

QRectF BackgroundItem::fullDrawingRect()
{
    double halfPenWidth = conf.penWidth / 2.0;
    return boundingRect().adjusted(halfPenWidth, halfPenWidth, -halfPenWidth, -halfPenWidth);
}

void BackgroundItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen(QColor(conf.backgrounds.at(conf.panel_background_id)->border_color));
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(conf.penWidth);
    painter->setPen(pen);
    painter->setBrush(conf.backgrounds.at(conf.panel_background_id)->background_color);
    painter->drawRect(fullDrawingRect());
}

class View : public QGraphicsView
{
public:
    View(QRect screenGeometry, struct sfdo *sfdo, QWidget *parent = 0);
    ~View();
    void addPlugin(int type, bool left_aligned, int &offset);

private:
    QWidget *m_parent;
    QGraphicsScene m_scene;
};

View::View(QRect screenGeometry, struct sfdo *sfdo, QWidget *parent) : QGraphicsView(parent)
{
    m_parent = parent;
    setScene(&m_scene);

    int width = screenGeometry.width();
    int height = conf.panel_height;

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

    info("load plugins");
    // Add plugins from right
    int offset_from_right = width;
    for (auto it = conf.panel_items_right.rbegin(); it != conf.panel_items_right.rend(); ++it) {
        addPlugin(*it, /* left_aligned */ false, offset_from_right);
    }

    // Add plugins from left
    int offset_from_left = 0;
    for (auto c : conf.panel_items_left) {
        addPlugin((int)c, /* left_aligned */ true, offset_from_left);
    }

    // The taskbar goes in the center and expands between the left/right hand plugins
    int taskbarWidth = offset_from_right - offset_from_left;
    if (taskbarWidth < 200)
        die("not enough space for taskbar; remove some plugins");
    Taskbar *taskbar = new Taskbar(&m_scene, conf.panel_height, taskbarWidth, sfdo);
    m_scene.addItem(taskbar);
    taskbar->setPos(offset_from_left, 0);

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

void View::addPlugin(int type, bool left_aligned, int &offset)
{
    switch (type) {
    case 'C': {
        ClockItem *clockItem = new ClockItem(this, conf.panel_height);
        m_scene.addItem(clockItem);
        if (!left_aligned)
            offset -= clockItem->boundingRect().width();
        clockItem->setPos(offset, 0);
        if (left_aligned)
            offset += clockItem->boundingRect().width();
        break;
    }
    default:
        break;
    }
}

Panel::Panel(QWidget *parent) : QMainWindow(parent)
{
    info("load sfdo resources");
    desktopEntryInit(&m_sfdo);

    info("init layer-shell surface");
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

    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    QString outputName;
    for (QScreen *s : screen->virtualSiblings()) {
        outputName = s->name();
        screenGeometry = s->geometry();
        if (outputName == conf.output) {
            break;
        }
    }
    info("use output '{}'", outputName.toStdString());

    QRect panelGeometry = screenGeometry;
    panelGeometry.setHeight(conf.panel_height);
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);

    QStackedLayout *layout = new QStackedLayout;
    m_centralWidget->setLayout(layout);

    View *view = new View(screenGeometry, &m_sfdo, m_centralWidget);
    layout->addWidget(view);

    setFixedSize(panelGeometry.size());
    setGeometry(panelGeometry);
    layerShell->setExclusiveZone(conf.panel_height);

    /*
     * Layer shell surfaces are tied to a particular screen once shown.
     * Hide and show to change screens
     */
    hide();
    show();

    resize(screenGeometry.width(), conf.panel_height);
}

Panel::~Panel()
{
    desktopEntryFinish(&m_sfdo);
}
