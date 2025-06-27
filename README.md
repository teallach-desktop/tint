# tint

## Why

Pursuing beauty

## What

A panel inspired by tint2

<img src="doc/tint.png"/>

## Dependencies

- meson, ninja, cmake
- qt6-base, qt6-wayland
- [layer-shell-qt]
- [wlr-protocols]
- [libsfdo]

On Arch Linux, run `sudo pacman -S layer-shell-qt wlr-protocols libsfdo`

[layer-shell-qt]: https://invent.kde.org/plasma/layer-shell-qt
[wlr-protocols]: https://repology.org/project/wlr-protocols/versions
[libsfdo]: https://gitlab.freedesktop.org/vyivel/libsfdo

## Build

```
meson setup build
meson compile -C build
```

## Run

```
build/tint -c tintrc
```

## References

- https://github.com/addy-dclxvi/tint2-theme-collections

