# tint

This is an experiment - which takes the form of a toy panel - to evaluate
[layer-shell-qt].

WARNING: It is extremely alpha and does not do anything useful.

Implementation:
- Render Qt QWindow (no QWidgets, QGraphicsScene or similar)
- Talk to [wlr-foreign-toplevel-management] protocol and show open clients

Dependencies:
- cmake, extra-cmake-modules
- qt5-{base,wayland}
- [layer-shell-qt] which is [packaged quite widely]

[layer-shell-qt]: https://invent.kde.org/plasma/layer-shell-qt
[packaged quite widely]: https://repology.org/project/layer-shell-qt/versions
[wlr-foreign-toplevel-management]: https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/blob/master/unstable/wlr-foreign-toplevel-management-unstable-v1.xml
