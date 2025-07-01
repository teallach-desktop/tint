<h3 align="center"><img src="doc/tint.png" alt="tint" height="64px"><br />tint </h3>
<p align="center">A Wayland panel inspired by tint2</p>

# Why

Pursuing beauty

# Obligatory Screenshot

<img src="doc/scrot1.png"/>

# Dependencies

- meson, ninja, cmake
- qt6-base, qt6-wayland
- [layer-shell-qt], [wlr-protocols], [libsfdo]

[layer-shell-qt]: https://invent.kde.org/plasma/layer-shell-qt
[wlr-protocols]: https://gitlab.freedesktop.org/wlroots/wlr-protocols
[libsfdo]: https://gitlab.freedesktop.org/vyivel/libsfdo

# Build

```
meson setup build
meson compile -C build
```

# Run

```
build/tint -c doc/tintrc
```

# References

- https://gitlab.com/o9000/tint2/-/blob/master/doc/tint2.md
- https://github.com/o9000/tint2/blob/master/doc/tint2.md
- https://code.google.com/archive/p/tint2/
- https://code.google.com/archive/p/tint2/wikis/Configure.wiki
- https://github.com/addy-dclxvi/tint2-theme-collections

# Distribution Specific Help

# Arch Linux

```bash
sudo pacman -S layer-shell-qt wlr-protocols libsfdo
```

