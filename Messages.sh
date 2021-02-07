#!/bin/sh
$EXTRACTRC `find . -name '*.ui' -o -name '*.rc' -o -name '*.ui'` >> rc.cpp
$XGETTEXT `find . -name '*.cpp' -o -name '*.h'` -o $podir/basket.pot
rm -f rc.cpp

## Welcome baskets
$XGETTEXT ./welcome/createWelcomeBaskets.py -o $podir/basketWelcome.pot
