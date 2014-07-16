cmake_minimum_required(VERSION 2.6)

find_package(X11 REQUIRED)
find_package(KDE4 REQUIRED)
find_package(QImageBlitz REQUIRED)
find_package(Gpgme)
find_package(Gpgmepp)
find_package(Soprano)
find_package(Nepomuk)
find_package(SharedDesktopOntologies)

# libgit2
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(Libgit2)
IF(LIBGIT2_FOUND)
  include_directories(${LIBGIT2_INCLUDE_DIR})
  add_definitions(-DWITH_LIBGIT2)
ELSE(LIBGIT2_FOUND)
  message("libgit2 not found, configuring without")
ENDIF(LIBGIT2_FOUND)



if (BUILD_KPARTS)
find_package(KdepimLibs REQUIRED)
include_directories(${KDEPIMLIBS_INCLUDE_DIRS})
endif (BUILD_KPARTS)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include(KDE4Defaults)

include(MacroLibrary)

IF(GPGME_FOUND)
  SET(HAVE_LIBGPGME 1)
  SET(LARGEFILE_SOURCE_1)
  ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
ELSE(GPGME_FOUND)
  MESSAGE("GPG not found, configuring without")
ENDIF(GPGME_FOUND)

IF(SHAREDDESKTOPONTOLOGIES_FOUND AND SOPRANO_FOUND AND NEPOMUK_FOUND)
  SET(HAVE_NEPOMUK 1)
ELSE(SHAREDDESKTOPONTOLOGIES_FOUND AND SOPRANO_FOUND AND NEPOMUK_FOUND)
  MESSAGE("Soprano and/or Nepomuk not found, configuring without")
ENDIF(SHAREDDESKTOPONTOLOGIES_FOUND AND SOPRANO_FOUND AND NEPOMUK_FOUND)

#TODO: find meinproc

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})


include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${X11_INCLUDES} )

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(po)
add_subdirectory(tags)
add_subdirectory(welcome)
add_subdirectory(backgrounds)
add_subdirectory(images)
add_subdirectory(file-integration)
