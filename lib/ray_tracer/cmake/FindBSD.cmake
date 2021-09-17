if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(VC_LIB_PATH_SUFFIX lib/x64)
else()
  set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

find_library(BSD_LIBRARY
  NAMES bsd
  HINTS
    ENV BSDDIR
    ${BSD_DIR}
  PATH_SUFFIXES lib ${VC_LIB_PATH_SUFFIX}
)

set(BSD_LIBRARIES ${BSD_LIBRARY})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(BSD
                                  REQUIRED_VARS BSD_LIBRARIES
                                  VERSION_VAR BSD_VERSION_STRING)

mark_as_advanced(BSD_LIBRARY BSD_INCLUDE_DIR)

