project(pruv_http)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

include_directories(include)


add_library(${PROJECT_NAME} SHARED
    include/pruv_http/http_loop.hpp
    include/pruv_http/status_codes.hpp
    include/pruv_http/url_coding.hpp
    include/pruv_http/url_fsm.hpp
    src/http_loop.cpp
    src/http_loop_no_rtti.cpp
    src/status_codes.cpp
    src/url_coding.cpp
    src/url_fsm.cpp
)

set_source_files_properties(
    src/http_loop_no_rtti.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)


add_executable(${PROJECT_NAME}_worker
    src/main.cpp
)

add_library(${PROJECT_NAME}_sample_handlers SHARED
    src/sample_handlers.cpp
)

file(GLOB_RECURSE pruv_http_test_sources_list test/*)
add_executable(${PROJECT_NAME}_test ${pruv_http_test_sources_list})


if (NOT Systemd_FOUND)
    find_package(Systemd REQUIRED)
endif()
include_directories(${SYSTEMD_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SYSTEMD_LIBRARIES})

if (NOT HttpParser_FOUND)
    find_package(HttpParser REQUIRED)
endif()
include_directories(${HTTP_PARSER_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${HTTP_PARSER_LIBRARY})

if (NOT pruv_FOUND)
    find_package(pruv REQUIRED)
endif()
include_directories(${pruv_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${pruv_LIBRARY})
target_link_libraries(${PROJECT_NAME}_worker ${pruv_LIBRARY})

if (NOT falloc_FOUND)
    find_package(falloc REQUIRED)
endif()
include_directories(${falloc_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${falloc_LIBRARY})

if (NOT Gtest_FOUND)
    find_package(Gtest REQUIRED)
endif()
include_directories(${GTEST_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME}_test ${GTEST_LIBRARY})
target_link_libraries(${PROJECT_NAME}_test ${GTEST_MAIN_LIBRARY})


target_link_libraries(${PROJECT_NAME} rt)

target_link_libraries(${PROJECT_NAME}_worker dl)
target_link_libraries(${PROJECT_NAME}_worker ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME}_worker ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}_sample_handlers ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME}_sample_handlers ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME}_test ${PROJECT_NAME})


set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET ${PROJECT_NAME}_worker PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${PROJECT_NAME}_worker PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME}_worker PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET ${PROJECT_NAME}_sample_handlers PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${PROJECT_NAME}_sample_handlers PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME}_sample_handlers PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET ${PROJECT_NAME}_test PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${PROJECT_NAME}_test PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME}_test PROPERTY CXX_STANDARD_REQUIRED ON)
