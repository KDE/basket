# BasKet Note Pads

## Purpose

This application provides as many baskets as you wish, and you can drag and drop
various objects (text, URLs, images, sounds...)  into its.

Objects can be edited, copied, dragged... So, you can arrange them as you want !

It allows to arrange notes, track to-dos and much more.

## Developers

Basket is build with the Qt6 and KDE Frameworks 6.

## Contact

If you have any questions, or would like to contribute (always welcome!) please
send me an email to the generall KDE development mailing list at
kde-devel@kde.org.

## Building/Installation

To build and install BasKet, follow these steps (this assumes you have the relevant
kf6 and qt6 development libraries and CMake):

```
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/.local/kde -DKDE_INSTALL_PLUGINDIR=~/.local/kde/lib64/qt6/plugins
make -j8
make -j8 install
```

## Flatpak

This application has a flatpak manifest.

```
flatpak-builder build-dir .flatpak-manifest.yaml
flatpak-builder --run build-dir .flatpak-manifest.yaml basket
```
