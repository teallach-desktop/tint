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
#include "sfdo.h"
#include "taskbar.h"

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
    info(" - load plugin-taskbar");
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
        info(" - load plugin-clock");
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

static void log_handler(enum sfdo_log_level level, const char *fmt, va_list args, void *tag)
{
    char buf[256];
    if (snprintf(buf, sizeof(buf), "[%s] %s", (const char *)tag, fmt) < (int)sizeof(buf)) {
        fmt = buf;
    }
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void Panel::desktopEntryInit(void)
{
    struct sfdo_basedir_ctx *basedir_ctx = sfdo_basedir_ctx_create();
    if (!basedir_ctx)
        die("sfdo_basedir_ctx_create()");
    m_sfdo.desktop_ctx = sfdo_desktop_ctx_create(basedir_ctx);
    if (!m_sfdo.desktop_ctx)
        die("sfdo_desktop_ctx_create()");
    m_sfdo.icon_ctx = sfdo_icon_ctx_create(basedir_ctx);
    if (!m_sfdo.icon_ctx)
        die("sfdo_icon_ctx_create()");
    enum sfdo_log_level level = SFDO_LOG_LEVEL_ERROR;
    sfdo_desktop_ctx_set_log_handler(m_sfdo.desktop_ctx, level, log_handler, (void *)"libsfdo");
    sfdo_icon_ctx_set_log_handler(m_sfdo.icon_ctx, level, log_handler, (void *)"libsfdo");
    char *locale = setlocale(LC_ALL, "");
    m_sfdo.desktop_db = sfdo_desktop_db_load(m_sfdo.desktop_ctx, locale);
    if (!m_sfdo.desktop_db)
        die("sfdo_desktop_db_load()");
    int load_options = SFDO_ICON_THEME_LOAD_OPTIONS_DEFAULT
            | SFDO_ICON_THEME_LOAD_OPTION_ALLOW_MISSING | SFDO_ICON_THEME_LOAD_OPTION_RELAXED;
    m_sfdo.icon_theme = sfdo_icon_theme_load(m_sfdo.icon_ctx, "Papirus", load_options);
    if (!m_sfdo.icon_theme)
        die("sfdo_icon_theme_load()");
    sfdo_basedir_ctx_destroy(basedir_ctx);
}

void Panel::desktopEntryFinish(void)
{
    sfdo_icon_theme_destroy(m_sfdo.icon_theme);
    sfdo_desktop_db_destroy(m_sfdo.desktop_db);
    sfdo_icon_ctx_destroy(m_sfdo.icon_ctx);
    sfdo_desktop_ctx_destroy(m_sfdo.desktop_ctx);
}

Panel::Panel(QWidget *parent) : QMainWindow(parent)
{
    info("load sfdo resources");
    desktopEntryInit();

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
        qDebug() << "name=" << outputName << "; geometry=" << s->geometry();
        screenGeometry = s->geometry();
        if (outputName == conf.output) {
            break;
        }
    }
    qDebug() << "Using output " << outputName;

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
    desktopEntryFinish();
}
