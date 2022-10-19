// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <QRasterWindow>

class Taskbar;

class Panel : public QRasterWindow
{
public:
    Panel();
    ~Panel();
    void addPlugin(Taskbar *taskbar);

private:
    Taskbar *m_taskbar;

protected:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
};
