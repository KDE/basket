#!/bin/sh
$EXTRACTRC `find . -name '*.ui' -o -name '*.rc'` >> rc.cpp
$XGETTEXT `find . -name '*.cpp' -o -name '*.h'` -o $podir/basket.pot
rm -f rc.cpp
