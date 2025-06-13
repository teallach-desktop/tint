// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QMainWindow>
#include "sfdo.h"

class Panel : public QMainWindow
{
public:
    Panel(QWidget *parent = nullptr);
    ~Panel();

    // libsfdo related
    void desktopEntryInit();
    void desktopEntryFinish();

private:
    QWidget *m_centralWidget;
    struct sfdo m_sfdo;
};
