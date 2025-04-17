// SPDX-License-Identifier: GPL-2.0-only
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QMetaEnum>
#include <LayerShellQt/shell.h>
#include <LayerShellQt/window.h>
#include <QtWaylandClient/private/qwayland-xdg-shell.h>
#include "conf.h"
#include "panel.h"
#include "taskbar.h"

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

    setAttribute(Qt::WA_AlwaysShowToolTips);

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    m_layout = new QHBoxLayout;
    centralWidget->setLayout(m_layout);

    m_layout->setSpacing(0);
    m_layout->setContentsMargins(QMargins(conf.pluginMarginLeft, conf.pluginMarginTop,
        conf.pluginMarginRight, conf.pluginMarginBottom));

    createActions();
    m_layout->addWidget(new Taskbar(this));

    constexpr int height = 32;
    QScreen * screen = QApplication::primaryScreen();
    QRect rect = screen->geometry();
    for (QScreen *s : screen->virtualSiblings())
    {
        // TODO: Add config option to set output
        qDebug() << "name=" << s->name() << "; geometry=" << s->geometry();
        rect = s->geometry();
    }
    rect.setHeight(0);
    qDebug() << "geometry=" << rect;
    setFixedSize(rect.size());
    setGeometry(rect);
    layerShell->setExclusiveZone(height);
}

Panel::~Panel() { }

void Panel::exit()
{
    QCoreApplication::quit();
}

void Panel::createActions()
{
    m_exitAct = new QAction("&Exit", this);
    connect(m_exitAct, &QAction::triggered, this, &Panel::exit);
}

