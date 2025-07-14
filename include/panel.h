// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QMainWindow>
#include <QTimer>
#include "resources.h"

class Panel : public QMainWindow
{
public:
    Panel(QWidget *parent = nullptr);
    ~Panel();

private:
    void updateGeometry();
    void updateGeometryDelayed();

    QTimer m_timer;
    QWidget *m_centralWidget;
    struct sfdo m_sfdo;
};
