BasKet Note Pads
================

Purpose
-------

This application provides as many baskets as you wish, and you can drag and drop
various objects (text, URLs, images, sounds...)  into its.

Objects can be edited, copied, dragged... So, you can arrange them as you want !

It allows to arrange notes, track to-dos and much more.

Developers
-----------

Basket is build with the Qt5 and KDE Frameworks 5.

Contact
-------
If you have any questions, or would like to contribute (always welcome!) please
send me an email to the generall KDE development mailing list at
kde-devel@kde.org.

Building/Installation
----------------------
To build and install BasKet, follow these steps (this assumes you have the relevant
kf5 and qt5 development libraries and CMake):

----
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=`kf5-config --prefix` -DKDE_INSTALL_QTPLUGINDIR=`qtpaths --plugin-dir` ..
make
# make install
----

Or you can try your luck with the installer script:

  ./installer
