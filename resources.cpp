// SPDX-License-Identifier: GPL-2.0-only
#include "log.h"
#include "resources.h"

static void log_handler(enum sfdo_log_level level, const char *fmt, va_list args, void *tag)
{
    char buf[256];
    if (snprintf(buf, sizeof(buf), "[%s] %s", (const char *)tag, fmt) < (int)sizeof(buf)) {
        fmt = buf;
    }
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void desktopEntryInit(struct sfdo *sfdo)
{
    struct sfdo_basedir_ctx *basedir_ctx = sfdo_basedir_ctx_create();
    if (!basedir_ctx)
        die("sfdo_basedir_ctx_create()");
    sfdo->desktop_ctx = sfdo_desktop_ctx_create(basedir_ctx);
    if (!sfdo->desktop_ctx)
        die("sfdo_desktop_ctx_create()");
    sfdo->icon_ctx = sfdo_icon_ctx_create(basedir_ctx);
    if (!sfdo->icon_ctx)
        die("sfdo_icon_ctx_create()");
    enum sfdo_log_level level = SFDO_LOG_LEVEL_ERROR;
    sfdo_desktop_ctx_set_log_handler(sfdo->desktop_ctx, level, log_handler, (void *)"libsfdo");
    sfdo_icon_ctx_set_log_handler(sfdo->icon_ctx, level, log_handler, (void *)"libsfdo");
    char *locale = setlocale(LC_ALL, "");
    sfdo->desktop_db = sfdo_desktop_db_load(sfdo->desktop_ctx, locale);
    if (!sfdo->desktop_db)
        die("sfdo_desktop_db_load()");
    int load_options = SFDO_ICON_THEME_LOAD_OPTIONS_DEFAULT
            | SFDO_ICON_THEME_LOAD_OPTION_ALLOW_MISSING | SFDO_ICON_THEME_LOAD_OPTION_RELAXED;
    sfdo->icon_theme = sfdo_icon_theme_load(sfdo->icon_ctx, "Papirus", load_options);
    if (!sfdo->icon_theme)
        die("sfdo_icon_theme_load()");
    sfdo_basedir_ctx_destroy(basedir_ctx);
}

void desktopEntryFinish(struct sfdo *sfdo)
{
    sfdo_icon_theme_destroy(sfdo->icon_theme);
    sfdo_desktop_db_destroy(sfdo->desktop_db);
    sfdo_icon_ctx_destroy(sfdo->icon_ctx);
    sfdo_desktop_ctx_destroy(sfdo->desktop_ctx);
}
