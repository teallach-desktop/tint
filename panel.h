// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QMainWindow>
#include <QHBoxLayout>

class Panel : public QMainWindow
{
public:
    Panel(QWidget *parent = nullptr);
    ~Panel();

private slots:
    void exit();

private:
    QHBoxLayout *m_layout;

    void createActions();
    QAction *m_exitAct;
};
