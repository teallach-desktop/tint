tint(1)

# NAME

tint - lightweight panel

# DESCRIPTION

Tint is a simple, lightweight panel for Wayland compositors which support the
wlr-layer-shell and wlr-foreign-toplevel-management protocols.

Features:

- Builtin taskbar and clock.
- High customizability via a plain text configuration file, including options
  for colors of fonts, icons, borders and backgrounds.

# SYNOPSIS

*tint [OPTION...]*

# OPTIONS

*-h|--help*
	Displays help on commandline options
*--help-all*
	Displays help, including generic Qt options
*-v|--version*
	Displays version information
*-o|--output <output>*
	Specify output (monitor)
*-c|--config <filename>*
	Specify config file

# CONFIGURATION

## Data types

*color*
	Hexadecimal RGB value using syntax *#rrggbb*

*opacity*
	Integer from 0 to 100, where 0 is fully transparent and 100 is fully
	opaque.

## Backgrounds

The tint config file starts with background object definitions which other
settings refer back to.

The reserved word *rounded* is used to denote the beginning of a new background
object section.

Each background object is identified by an integer starting from 1 (1, 2, ...)
based on the order they appear in the config file. Identifier 0 refers to a
special background which is fully transparent.

*rounded = <radius>*
	Corner radius expressed in number of pixels.

*background_color = <color> <opacity>*
	Background color.

*border_color = <color> <opacity>*
	Border color.

For example, the following config defines two background objects:

```
# Background 1
rounded = 1
background_color = #282828 100
border_color = #000000 0

# Background 2
rounded = 1
background_color = #f6b655 90
border_color = #cccccc 40
```

Later in the config file, these backgrounds can be applied using the assigned
id, for example:

```
panel_background_id = 1
taskbar_background_id = 0
task_background_id = 0
task_active_background_id = 2
clock_background_id = 0
```

## Panel

*panel_background_id = <id>*
	Which background to use for the panel. Default is 0.

*panel_items = string*
	Which plugins to be shown and their order (from left to right). Each
	letter refers to an item as defined below. Default is *TC*.
	- *T* shows the taskbar
	- *C* shows the clock

*panel_size = \_ <height>*
	Panel height in pixels. Default is 36.

## Taskbar

*taskbar_background_id = <id>*
	Which background to use for the taskbar. Default is 0.

*taskbar_padding = <horizontal-padding> <vertical-padding> <spacing>*
	Taskbar padding and spacing of tasks within it. Default is 0
	for all.

	```
	taskbar
	╔═════════════════════════╗
	║      2           2      ║
	║   ┌─────┐     ┌─────┐   ║
	║ 1 │task │  3  │task │ 1 ║
	║   └─────┘     └─────┘   ║
	║      2           2      ║
	╚═════════════════════════╝
	1. horizontal padding
	2. vertical padding
	3. spacing
	```

## Taskbar Task

A task is the graphical artefact representing one application within the
taskbar.

*task_maximum_size = <width> \_*
	Maximum width of tasks to limit their size. Use *width=0* to use the
	full taskbar width. Default is 0.

*task_font = <font> \_ <size>*
	Font and size to use for tasks

*task_font_color = <color> <opacity>*
	Font color to use for tasks

*task_background_id = <id>*
	Which background to use for non selected tasks

*task_active_background_id = <id>*
	Which background to use for non selected tasks

## Clock

*time1_font = <font> \_ <size>*
	Font and size to use for the clock

*clock_font_color = <color> <opacity>*
	Font color to use for the clock

*clock_background_id = <id>*
	Which background to use for the clock

