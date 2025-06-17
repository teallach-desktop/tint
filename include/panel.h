// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QMainWindow>
#include "resources.h"

class Panel : public QMainWindow
{
public:
    Panel(QWidget *parent = nullptr);
    ~Panel();

private:
    QWidget *m_centralWidget;
    struct sfdo m_sfdo;
};
