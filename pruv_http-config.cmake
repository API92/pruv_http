# pruv_http-config
# --------------
#
# This module defines
#
# ::
#
#   pruv_http_FOUND - Set to true when pruv_http is found.
#   pruv_http_INCLUDE_DIR - the directory of the pruv_http headers
#   pruv_http_LIBRARY - the pruv_http library needed for linking

find_path(pruv_http_INCLUDE_DIR
    NAME pruv_http/http_loop.hpp
    PATHS ${CMAKE_CURRENT_LIST_DIR}/include)

find_library(pruv_http_LIBRARY
    NAMES pruv_http libpruv_http
    PATHS ${CMAKE_CURRENT_LIST_DIR}
    PATH_SUFFIXES bin/${CMAKE_CXX_COMPILER_ID}/${CMAKE_BUILD_TYPE})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pruv_http
    "Could NOT find pruv_http, try to set the path to the pruv_http root folder in the variable pruv_http_DIR"
    pruv_http_LIBRARY
    pruv_http_INCLUDE_DIR)
