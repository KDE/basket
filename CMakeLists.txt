PROJECT(basket)

# Find the requisite packages
FIND_PACKAGE(KDE3 REQUIRED)
INCLUDE_DIRECTORIES(${KDE3_INCLUDE_DIRS})
ADD_DEFINITIONS(${QT_DEFINITIONS} ${KDE3_DEFINITIONS})
LINK_DIRECTORIES(${QT_LIB_DIR} ${KDE3_LIB_DIR})

FIND_LIBRARY(GPGME gpgme)
IF(NOT GPGME-NOTFOUND)
  SET(HAVE_LIBGPGME 1)
  SET(GPGME_LIB gpgme)
  ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
ELSE(NOT GPGME-NOTFOUND)
  MESSAGE("GPG not found, configuring without")
ENDIF(NOT GPGME-NOTFOUND)

#TODO: use KDE's settings for this
FIND_PROGRAM(MEINPROC meinproc)
set(KDE4_MEINPROC_EXECUTABLE ${MEINPROC})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_SUBDIRECTORY(doc)
ADD_SUBDIRECTORY(src)