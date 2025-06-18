// SPDX-License-Identifier: GPL-2.0-only
#include <cstring>
#include <cmath>
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

/* Return the length of a filename minus any known extension */
static size_t length_without_extension(const char *name)
{
    size_t len = strlen(name);
    if (len >= 4 && name[len - 4] == '.') {
        const char *ext = &name[len - 3];
        if (!strcmp(ext, "png") || !strcmp(ext, "svg") || !strcmp(ext, "xpm")) {
            len -= 4;
        }
    }
    return len;
}

static std::string process_rel_name(const char *icon_name, struct sfdo *sfdo, int size, int scale)
{
    std::string iconpath;
    int lookup_options = SFDO_ICON_THEME_LOOKUP_OPTIONS_DEFAULT;

    /*
     * Relative icon names are not supposed to include an extension, but some .desktop files include
     * one anyway. libsfdo does not allow this in lookups, so strip the extension first.
     */
    size_t name_len = length_without_extension(icon_name);
    struct sfdo_icon_file *icon_file = sfdo_icon_theme_lookup(sfdo->icon_theme, icon_name, name_len,
                                                              size, scale, lookup_options);

    if (icon_file && icon_file != SFDO_ICON_FILE_INVALID)
        iconpath = sfdo_icon_file_get_path(icon_file, NULL);

    sfdo_icon_file_destroy(icon_file);
    return iconpath;
}

static struct sfdo_desktop_entry *get_db_entry_by_id_fuzzy(struct sfdo_desktop_db *db,
                                                           const char *app_id)
{
    size_t n_entries;
    struct sfdo_desktop_entry **entries = sfdo_desktop_db_get_entries(db, &n_entries);

    for (size_t i = 0; i < n_entries; i++) {
        struct sfdo_desktop_entry *entry = entries[i];
        const char *desktop_id = sfdo_desktop_entry_get_id(entry, NULL);
        /* Get portion of desktop ID after last '.' */
        const char *dot = strrchr(desktop_id, '.');
        const char *desktop_id_base = dot ? (dot + 1) : desktop_id;

        if (!strcasecmp(app_id, desktop_id_base)) {
            return entry;
        }

        /* sfdo_desktop_entry_get_startup_wm_class() asserts against APPLICATION */
        if (sfdo_desktop_entry_get_type(entry) != SFDO_DESKTOP_ENTRY_APPLICATION) {
            continue;
        }

        /* Try desktop entry's StartupWMClass also */
        const char *wm_class = sfdo_desktop_entry_get_startup_wm_class(entry, NULL);
        if (wm_class && !strcasecmp(app_id, wm_class)) {
            return entry;
        }
    }

    /* Try matching partial strings - catches GIMP, among others */
    for (size_t i = 0; i < n_entries; i++) {
        struct sfdo_desktop_entry *entry = entries[i];
        const char *desktop_id = sfdo_desktop_entry_get_id(entry, NULL);
        const char *dot = strrchr(desktop_id, '.');
        const char *desktop_id_base = dot ? (dot + 1) : desktop_id;
        int alen = strlen(app_id);
        int dlen = strlen(desktop_id_base);

        if (!strncasecmp(app_id, desktop_id, alen > dlen ? dlen : alen)) {
            return entry;
        }
    }

    return NULL;
}

static struct sfdo_desktop_entry *get_desktop_entry(struct sfdo *sfdo, const char *app_id)
{
    struct sfdo_desktop_entry *entry =
            sfdo_desktop_db_get_entry_by_id(sfdo->desktop_db, app_id, SFDO_NT);
    if (!entry) {
        entry = get_db_entry_by_id_fuzzy(sfdo->desktop_db, app_id);
    }
    return entry;
}

static std::string get_icon_path(struct sfdo *sfdo, const char *icon_name, int size, float scale)
{
    std::string iconpath;

    if (!icon_name || !*icon_name)
        return iconpath;

    int lookup_scale = std::max((int)scale, 1);
    int lookup_size = lroundf(size * scale / lookup_scale);

    if (icon_name[0] == '/') {
        iconpath = std::string(icon_name);
    } else {
        iconpath = process_rel_name(icon_name, sfdo, lookup_size, lookup_scale);
    }
    if (iconpath.empty()) {
        info("failed to load icon file {}", icon_name);
    }
    return iconpath;
}

std::string load_icon_from_app_id(struct sfdo *sfdo, const char *app_id, int size, float scale)
{
    std::string iconpath;

    if (!app_id || !*app_id)
        return iconpath;

    struct sfdo_desktop_entry *entry = get_desktop_entry(sfdo, app_id);
    const char *icon_name = entry ? sfdo_desktop_entry_get_icon(entry, NULL) : NULL;
    iconpath = get_icon_path(sfdo, icon_name, size, scale);

    /*
     * Fallback on using the app_id if 'Icon' is not defined in .desktop file or the icon could not
     * be loaded. This is not spec compliant, but does help find icons in some circumstances.
     */
    if (iconpath.empty()) {
        iconpath = get_icon_path(sfdo, app_id, size, scale);
    }

    return iconpath;
}
