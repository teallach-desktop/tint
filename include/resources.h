// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <sfdo-desktop.h>
#include <sfdo-icon.h>
#include <sfdo-basedir.h>

struct sfdo {
    struct sfdo_desktop_ctx *desktop_ctx;
    struct sfdo_icon_ctx *icon_ctx;
    struct sfdo_desktop_db *desktop_db;
    struct sfdo_icon_theme *icon_theme;
};

void desktopEntryInit(struct sfdo *sfdo);
void desktopEntryFinish(struct sfdo *sfdo);
