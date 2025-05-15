// SPDX-License-Identifier: GPL-2.0-only
#include <fstream>
#include <memory>
#include <print>
#include <ranges>
#include <regex>
#include <string>
#include <vector>
#include "conf.h"

struct conf conf;

Background::Background(void)
{
    rounded = 0;
    background_color = QColor("#00000000");
}

Background::~Background(void) { };

static int current_background_index = 0;

static std::vector<std::string> split(std::string s, char delim)
{
    auto has_content = [](auto const &s) { return s.size() > 0; };
    auto parts = s | std::views::split(delim) | std::views::filter(has_content)
            | std::ranges::to<std::vector<std::string>>();
    return parts;
}

static QColor getColor(std::string value)
{
    auto parts = split(value, ' ');
    // TODO: validate

    std::string rrggbb = parts.at(0).erase(0, 1);
    std::string aa = std::format("{:x}", 255 * std::stoi(parts.at(1)) / 100);
    std::string color = "#" + aa + rrggbb;
    return QColor(QString::fromStdString(color));
}

std::string trim(std::string s)
{
    return regex_replace(s, std::regex("(^[ ]+)|([ ]+$)"), "");
}

static void process_line(std::string line)
{
    auto parts = line | std::views::split('=') | std::ranges::to<std::vector<std::string>>();
    if (parts.size() < 2) {
        return;
    }

    auto key = trim(parts[0]);
    auto value = trim(parts[1]);
    if (key == "panel_items") {
        conf.panel_items = key;
    } else if (key == "panel_size") {
        // validate
        auto parts = split(value, ' ');
        conf.panel_height = std::stoi(parts.at(1));
    } else if (key == "panel_background_id") {
        // TODO: check that ID is valid
        conf.panel_background_id = std::stoi(value);
    } else if (key == "taskbar_padding") {
        // validate
        auto parts = split(value, ' ');
        conf.taskbar_padding_horizontal = std::stoi(parts.at(0));
        conf.taskbar_padding_vertical = std::stoi(parts.at(1));
        conf.taskbar_padding_spacing = std::stoi(parts.at(2));
    } else if (key == "task_maximum_size") {
        auto parts = split(value, ' ');
        conf.task_maximum_size = std::stoi(parts.at(0));
    } else if (key == "task_background_id") {
        conf.task_background_id = std::stoi(value);
    } else if (key == "task_active_background_id") {
        conf.task_active_background_id = std::stoi(value);
    } else if (key == "rounded") {
        // 'rounded' is special because it defines the start of a background object section
        conf.backgrounds.push_back(std::make_unique<Background>());
        ++current_background_index;
        conf.backgrounds.at(current_background_index)->rounded = std::stoi(value);
    } else if (key == "background_color") {
        conf.backgrounds.at(current_background_index)->background_color = getColor(value);
    }
}

static void parse(std::string filename)
{
    std::ifstream file(filename);
    std::string line;
    if (!file.is_open()) {
        std::println(stderr, "cannot open file '{}'", filename);
        return;
    }
    while (std::getline(file, line)) {
        process_line(line);
    }
    file.close();
}

void confInit(QString filename)
{
    conf.panel_items = "LTC";
    conf.panel_height = 30;

    conf.taskbar_padding_horizontal = 0;
    conf.taskbar_padding_vertical = 0;
    conf.taskbar_padding_spacing = 0;

    // background_id 0 refers to a special background which is fully transparent
    conf.backgrounds.push_back(std::make_unique<Background>());

    // TOOD: Init all values

    parse(filename.toStdString());
}

void confSetOutput(QString output)
{
    conf.output = output;
}
