# - Try to find the libgit2 library
# Once done this will define
#
#  LIBGIT2_FOUND - System has libgit2
#  LIBGIT2_INCLUDE_DIR - The libgit2 include directory
#  LIBGIT2_LIBRARIES - The libraries needed to use libgit2
#  LIBGIT2_DEFINITIONS - Compiler switches required for using libgit2


# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
FIND_PACKAGE(PkgConfig)
PKG_SEARCH_MODULE(PC_LIBGIT2 libgit2)

SET(LIBGIT2_DEFINITIONS ${PC_LIBGIT2_CFLAGS_OTHER})

FIND_PATH(LIBGIT2_INCLUDE_DIR NAMES git2.h
   HINTS
   ${PC_LIBGIT2_INCLUDEDIR}
   ${PC_LIBGIT2_INCLUDE_DIRS}
)

FIND_LIBRARY(LIBGIT2_LIBRARIES NAMES git2
   HINTS
   ${PC_LIBGIT2_LIBDIR}
   ${PC_LIBGIT2_LIBRARY_DIRS}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libgit2 DEFAULT_MSG LIBGIT2_LIBRARIES LIBGIT2_INCLUDE_DIR)

MARK_AS_ADVANCED(LIBGIT2_INCLUDE_DIR LIBGIT2_LIBRARIES)

IF(Libgit2_FIND_VERSION)
    # Retrieve the actual version
    EXECUTE_PROCESS(
        COMMAND pkg-config --modversion libgit2
        OUTPUT_VARIABLE LIBGIT2_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Check versions
    IF(NOT LIBGIT2_VERSION VERSION_GREATER_EQUAL ${Libgit2_FIND_VERSION})
      if ( NOT Libgit2_FIND_QUIETLY )
        MESSAGE(WARNING "libgit2 version ${Libgit2_FIND_VERSION} or newer required, found ${LIBGIT2_VERSION}")
      endif()
      SET(LIBGIT2_FOUND FALSE)
    ENDIF()
ENDIF()

