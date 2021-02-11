find_library(XQILLA_LIB name xqilla
    PATHS ${CMAKE_PREFIX_PATH}/lib/
    REQUIRED)

find_path(XQILLA_INCLUDE_DIR
    name xqilla-dom3.hpp
    PATHS ${CMAKE_PREFIX_PATH}/include/xqilla
    REQUIRED)


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Xqilla  DEFAULT_MSG
                                  XQILLA_LIB XQILLA_INCLUDE_DIR)

mark_as_advanced(XQILLA_INCLUDE_DIR XQILLA_LIB )


set(XQILLA_LIBRARIES ${XQILLA_LIB} )
set(XQILLA_INCLUDE_DIRS ${XQILLA_INCLUDE_DIR} )
