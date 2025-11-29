# SPDX-FileCopyrightText: (C) 2008 Kelvie Wong <kelvie@ieee.org>
# SPDX-FileCopyrightText: (C) 2014 Gleb Baryshev <gleb.baryshev@gmail.com>
# SPDX-FileCopyrightText: (C) 2018 Luigi Toscano <luigi.toscano@tiscali.it>
#
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.16)

project(Basket VERSION 2.49.90)

set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.2.0")

# Options
option(BUILD_DEVTOOLS "Enabled Devtools" ON)
option(ENABLE_GPG "Enabled GPG Support" OFF)
option(ENABLE_GIT "Enabled Git Support" ON)
option(DEBUG_PIPE "Enabled Named Debugging Pipe" OFF)

# for local usage only
# set(CMAKE_BUILD_PARALLEL_LEVEL 12)

# do we use a GNU compiler?
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fno-delete-null-pointer-checks")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# ECM
find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules)

include(KDEInstallDirs)
include(ECMSetupVersion)
include(KDECMakeSettings)
include(KDEClangFormat)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(KDEGitCommitHooks)
include(ECMDeprecationSettings)
include(ECMFeatureSummary)

ecm_setup_version(
    PROJECT
    VARIABLE_PREFIX BASKET
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/basket_version.h")

# GPG
if (ENABLE_GPG)
    find_package(PkgConfig)
    pkg_check_modules(gpgme REQUIRED IMPORTED_TARGET gpgme>=1.23.2)
    set(HAVE_LIBGPGME 1)
endif()

# libgit2
if (ENABLE_GIT)
    find_package(Libgit2 1.8.4 QUIET)
    set_package_properties(Libgit2 PROPERTIES DESCRIPTION "A library to manipulate git repositories"
        PURPOSE "Add git support to Basket"
        URL "https://libgit2.org"
        TYPE OPTIONAL
    )
endif()

if (LIBGIT2_FOUND)
    MESSAGE(STATUS "FOUND: Git2")
    include_directories(${LIBGIT2_INCLUDE_DIR})
    add_definitions(-DWITH_LIBGIT2)
else()
    if (NOT ENABLE_GIT)
        MESSAGE(VERBOSE "Git2 disabled, configuring without")
    else()
        MESSAGE(WARNING "Git2 not found, configuring without")
    endif()
endif()

# Debugging Pipe
if (DEBUG_PIPE)
    MESSAGE(STATUS "Enable Named Debugging Pipe")
    add_definitions(-DDEBUG_PIPE)
endif()

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Concurrent
    Core
    DBus
    Gui
    Multimedia
    Widgets
    Xml
    Core5Compat
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Codecs
    ColorScheme
    Completion
    Config
    ConfigWidgets #KCM
    CoreAddons
    Crash
    DBusAddons
    FileMetaData
    GlobalAccel
    GuiAddons
    I18n
    IconThemes
    JobWidgets
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
find_package(KF6DocTools ${KF_MIN_VERSION})
set_package_properties(KF6DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)

#Make libbasketcommon search for translations in basket.mo
add_definitions(-DTRANSLATION_DOMAIN=\"basket\")
add_definitions(-DQT_NO_KEYWORDS)

ecm_set_disabled_deprecation_versions(QT 5.15.0
    KF 6.10.0
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(KF6DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()


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
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

