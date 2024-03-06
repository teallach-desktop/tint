// SPDX-License-Identifier: GPL-2.0-only
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QDebug>
#include <QMenu>
#include <QMetaEnum>
#include <LayerShellQt/shell.h>
#include <LayerShellQt/window.h>
#include "panel.h"
#include "taskbar.h"

Panel::Panel()
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

    layerShell->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivity(0));

    setAttribute(Qt::WA_AlwaysShowToolTips);

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    m_layout = new QHBoxLayout;
    centralWidget->setLayout(m_layout);

    m_layout->setContentsMargins(QMargins(0, 0, 0, 0));
    m_layout->setSpacing(0);

    createActions();
    m_layout->addWidget(new Taskbar(this));

    constexpr int height = 32;
    resize(0, height);
    qDebug() << "size=" << this->size();
    layerShell->setExclusiveZone(height);
}

Panel::~Panel() { }

void Panel::exit()
{
    QCoreApplication::quit();
}

void Panel::createActions()
{
    m_fooAct = new QAction("&Foo", this);
    m_barAct = new QAction("&Bar", this);
    m_exitAct = new QAction("&Exit", this);
    connect(m_exitAct, &QAction::triggered, this, &Panel::exit);
}

void Panel::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(m_fooAct);
    menu.addAction(m_barAct);
    menu.addAction(m_exitAct);
    menu.exec(event->globalPos());
}
