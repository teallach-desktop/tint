// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QMainWindow>
#include <QHBoxLayout>

class Panel : public QMainWindow
{
public:
    Panel();
    ~Panel();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void exit();

private:
    QHBoxLayout *m_layout;

    void createActions();
    QAction *m_fooAct;
    QAction *m_barAct;
    QAction *m_exitAct;
};
