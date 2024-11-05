# SPDX-FileCopyrightText: (C) 2008 Kelvie Wong <kelvie@ieee.org>
# SPDX-FileCopyrightText: (C) 2014 Gleb Baryshev <gleb.baryshev@gmail.com>
# SPDX-FileCopyrightText: (C) 2018 Luigi Toscano <luigi.toscano@tiscali.it>
#
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.5)

project(Basket VERSION 2.49.90)

set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.2.0")

set(APP_SUFFIX 6)
set(LIB_PREFIX Qt6::)

# Options
option(ENABLE_GPG "Enabled GPG Support" OFF)
option(ENABLE_GIT "Enabled Git Support" ON)

# for local usage only
# set(CMAKE_BUILD_PARALLEL_LEVEL 12)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fno-delete-null-pointer-checks")
    # TODO: after removing kdelibs4support this could also be removed
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# ECM
include(FeatureSummary)
find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules)

include(KDEInstallDirs6)
include(ECMSetupVersion)
include(KDECMakeSettings)
include(KDEClangFormat)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)

ecm_setup_version(
    PROJECT
    VARIABLE_PREFIX BASKET
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/basket_version.h")

# GPG
if (ENABLE_GPG)
    find_package(Gpgme 1.1.7)
endif()

# libgit2
if (ENABLE_GIT)
    find_package(Libgit2 1.8.2 QUIET)
    set_package_properties(Libgit2 PROPERTIES DESCRIPTION "A library to manipulate git repositories"
        PURPOSE "Add git support to Basket"
        URL "https://libgit2.org"
        TYPE OPTIONAL
    )
endif()

if (LIBGIT2_FOUND)
    MESSAGE("Git2 enabled and found")
    include_directories(${LIBGIT2_INCLUDE_DIR})
    add_definitions(-DWITH_LIBGIT2)
else()
    if (NOT ENABLE_GIT)
        MESSAGE("Git2 disabled, configuring without")
    else()
        MESSAGE("Git2 not found, configuring without")
    endif()
endif()

# TODO make X11 optional
find_package(X11 REQUIRED)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Concurrent
    Core
    DBus
    Gui
    Widgets
    Xml
    Core5Compat
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Codecs
    Completion
    Config
    ConfigWidgets #KCM
    CoreAddons
    Crash
    DBusAddons
    DocTools
    FileMetaData
    GlobalAccel
    GuiAddons
    I18n
    IconThemes
    KCMUtils
    KIO
    Notifications
    Parts
    Service
    TextWidgets
    WidgetsAddons
    WindowSystem
    XmlGui
)


find_package(Phonon4Qt6 REQUIRED)

if (GPGME_FOUND)
    MESSAGE("GPG enabled and found")
    set(HAVE_LIBGPGME 1)
    SET(LARGEFILE_SOURCE_1)
    ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
else()
    if (NOT ENABLE_GPG)
        MESSAGE("GPG disabled, configuring without")
    else()
        MESSAGE("GPG not found, configuring without")
    endif()
endif()


#TODO: find meinproc

#Make libbasketcommon search for translations in basket.mo
add_definitions(-DTRANSLATION_DOMAIN=\"basket\")
add_definitions(-DQT_NO_KEYWORDS)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(tags)
add_subdirectory(welcome)
add_subdirectory(backgrounds)
add_subdirectory(images)
add_subdirectory(file-integration)

if(BUILD_DEVTOOLS)
    add_subdirectory(devtools)
endif()

ki18n_install(po)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
