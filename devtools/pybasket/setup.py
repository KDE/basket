#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2021 Sebastian Engel <kde@sebastianengel.eu>
#
# SPDX-License-Identifier: GPL-2.0-or-later

from setuptools import setup

setup(
  name='pybasket',
  version='0.0.1',
  description='a simple package to create BasKet directories',
  license='GPL-2.0-or-later',
  author='Sebastian Engel',
  author_email='kde@sebastianengel.eu',
  url='https://invent.kde.org/utilities/basket',
  packages=['pybasket'],
  install_requires=[
    'datetime',
    'uuid',
    ],
)

# vim: sw=2 ts=2
