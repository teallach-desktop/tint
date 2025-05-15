# tint

## Why

Pursuing beauty.

## What

A panel inspired by tint2.

Aims to re-produce these one day:
- https://github.com/addy-dclxvi/tint2-theme-collections

## Dependencies

- meson, ninja, cmake
- qt6-base, qt6-wayland
- [layer-shell-qt]
- [wlr-protocols]

On Arch Linux, sun `sudo pacman -S layer-shell-qt wlr-protocols`

[layer-shell-qt]: https://invent.kde.org/plasma/layer-shell-qt
[wlr-protocols]: https://repology.org/project/wlr-protocols/versions

## Build

```
meson setup build
meson compile -C build
```

## Run

```
build/tint -c tintrc
```


