// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QMainWindow>

class Panel : public QMainWindow
{
public:
    Panel(QWidget *parent = nullptr);
    ~Panel();

private:
    QWidget *m_centralWidget;
};
