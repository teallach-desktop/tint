// SPDX-License-Identifier: GPL-2.0-only
#include <fstream>
#include <memory>
#include <ranges>
#include <regex>
#include <string>
#include <vector>
#include "conf.h"
#include "log.h"

struct conf conf;

Background::Background(void)
{
    rounded = 0;
    background_color = QColor("#00000000");
    border_color = QColor("#00000000");
}

Background::~Background(void) { };

static int current_background_index = 0;

static std::vector<std::string> split(const std::string &s, char delim)
{
    auto has_content = [](auto const &s) { return s.size() > 0; };
    auto parts = s | std::views::split(delim) | std::views::filter(has_content)
            | std::ranges::to<std::vector<std::string>>();
    return parts;
}

static QColor getColor(std::string value)
{
    auto parts = split(value, ' ');
    if (parts.size() != 2)
        die("incorrect color syntax '{}'; expected '#rrggbb aaa'", value);
    std::string rrggbb = parts.at(0).erase(0, 1);
    std::string aa = std::format("{:x}", 255 * std::stoi(parts.at(1)) / 100);
    if (aa.length() != 2)
        aa.insert(0, "0");
    std::string color = "#" + aa + rrggbb;
    return QColor(QString::fromStdString(color));
}

static QFont getFont(std::string value)
{
    auto parts = split(value, ' ');
    if (parts.size() != 3)
        die("incorrect font syntax '{}'; expected '[family] [style] [size]'", value);
    QFont font;
    font.setFamily(QString::fromStdString(parts.at(0)));
    font.setPointSize(std::stoi(parts.at(2)));
    return font;
}

static struct padding getPadding(std::string value)
{
    auto parts = split(value, ' ');
    if (parts.size() != 3)
        die("incorrect padding syntax '{}'; expected '[horizontal] [vertical] [spacing]'", value);
    struct padding padding{
        .horizontal = std::stoi(parts.at(0)),
        .vertical = std::stoi(parts.at(1)),
        .spacing = std::stoi(parts.at(2)),
    };
    return padding;
}

std::string trim(std::string s)
{
    return regex_replace(s, std::regex("(^[ ]+)|([ ]+$)"), "");
}

uint32_t getBackgroundId(std::string value)
{
    uint32_t id = std::stoi(value);
    if (id > conf.backgrounds.size() - 1)
        die("background_id '{}' not defined", id);
    return id;
}

static void process_line(const std::string &line)
{
    auto hunks = line | std::views::split('=') | std::ranges::to<std::vector<std::string>>();
    if (hunks.size() < 2) {
        return;
    }

    auto key = trim(hunks[0]);
    auto value = trim(hunks[1]);

    // Panel
    if (key == "panel_items") {
        if (!value.contains("T"))
            die("no 'T' in panel_items");
        auto parts = value | std::views::split('T') | std::ranges::to<std::vector<std::string>>();
        conf.panel_items_left = parts.at(0);
        conf.panel_items_right = parts.at(1);
    } else if (key == "panel_size") {
        auto parts = split(value, ' ');
        if (parts.size() != 2)
            die("incorrect syntax '{}={}'; expected two space separated values", key, value);
        conf.panel_height = std::stoi(parts.at(1));
    } else if (key == "panel_background_id") {
        conf.panel_background_id = getBackgroundId(value);

        // Taskbar
    } else if (key == "taskbar_background_id") {
        conf.taskbar_background_id = getBackgroundId(value);
    } else if (key == "taskbar_padding") {
        conf.taskbar_padding = getPadding(value);

        // Task
    } else if (key == "task_maximum_size") {
        auto parts = split(value, ' ');
        if (parts.size() != 2)
            die("incorrect syntax '{}={}'; expected two space separated values", key, value);
        conf.task_maximum_size = std::stoi(parts.at(0));
    } else if (key == "task_font") {
        conf.task_font = getFont(value);
    } else if (key == "task_font_color") {
        conf.task_font_color = getColor(value);
    } else if (key == "task_background_id") {
        conf.task_background_id = getBackgroundId(value);
    } else if (key == "task_active_background_id") {
        conf.task_active_background_id = getBackgroundId(value);

        // Clock
    } else if (key == "clock_background_id") {
        conf.clock_background_id = getBackgroundId(value);
    } else if (key == "time1_font") {
        conf.time1_font = getFont(value);
    } else if (key == "clock_font_color") {
        conf.clock_font_color = getColor(value);

        // Backgrounds
    } else if (key == "rounded") {
        // 'rounded' is special because it defines the start of a background object section
        conf.backgrounds.push_back(std::make_unique<Background>());
        ++current_background_index;
        conf.backgrounds.at(current_background_index)->rounded = std::stoi(value);
    } else if (key == "background_color") {
        conf.backgrounds.at(current_background_index)->background_color = getColor(value);
    } else if (key == "border_color") {
        conf.backgrounds.at(current_background_index)->border_color = getColor(value);
    }
}

static void parse(std::string filename)
{
    std::ifstream file(filename);
    std::string line;
    if (!file.is_open())
        warn("cannot open file '{}'", filename);
    while (std::getline(file, line))
        process_line(line);
    file.close();
}

void confInit(QString filename)
{
    // Panel
    conf.panel_items_left = "T";
    conf.panel_items_right = "C";
    conf.panel_height = 30;

    // Taskbar
    conf.taskbar_padding = { 0 };

    conf.task_font = QFont("Sans", 10);
    conf.task_font_color = QColor("#ffffff");

    // Clock
    conf.time1_font = QFont("Sans", 10);
    conf.clock_font_color = QColor("#ffffff");

    // Temporary global settings
    conf.penWidth = 1.0;
    conf.verbosity = 0;

    // background_id 0 refers to a special background which is fully transparent
    conf.backgrounds.push_back(std::make_unique<Background>());

    // TOOD: Init all background values

    parse(filename.toStdString());
}

void confSetOutput(QString output)
{
    conf.output = output;
}

void confSetVerbosity(int verbosity)
{
    conf.verbosity = verbosity;
}
