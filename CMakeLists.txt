project(pruv_http)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

include_directories(include)

if (NOT Systemd_FOUND)
    find_package(Systemd REQUIRED)
endif()
include_directories(${SYSTEMD_INCLUDE_DIRS})

if (NOT HttpParser_FOUND)
    find_package(HttpParser REQUIRED)
endif()
include_directories(${HTTP_PARSER_INCLUDE_DIR})

if (NOT pruv_FOUND)
    find_package(pruv REQUIRED)
endif()
include_directories(${pruv_INCLUDE_DIR})

if (NOT falloc_FOUND)
    find_package(falloc REQUIRED)
endif()
include_directories(${falloc_INCLUDE_DIR})


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


target_link_libraries(${PROJECT_NAME} ${HTTP_PARSER_LIBRARY})
target_link_libraries(${PROJECT_NAME} rt)
target_link_libraries(${PROJECT_NAME} ${SYSTEMD_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${pruv_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${falloc_LIBRARY})

target_link_libraries(${PROJECT_NAME}_worker dl)
target_link_libraries(${PROJECT_NAME}_worker ${pruv_LIBRARY})
target_link_libraries(${PROJECT_NAME}_worker ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME}_worker ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}_sample_handlers ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME}_sample_handlers ${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET ${PROJECT_NAME}_worker PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${PROJECT_NAME}_worker PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME}_worker PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET ${PROJECT_NAME}_sample_handlers PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${PROJECT_NAME}_sample_handlers PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME}_sample_handlers PROPERTY CXX_STANDARD_REQUIRED ON)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pruv_http> ${PROJECT_SOURCE_DIR}/bin/${CMAKE_CXX_COMPILER_ID}/${CMAKE_BUILD_TYPE}/$<TARGET_FILE_NAME:pruv_http>
    VERBATIM
)

add_custom_command(TARGET ${PROJECT_NAME}_worker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pruv_http_worker> ${PROJECT_SOURCE_DIR}/bin/${CMAKE_CXX_COMPILER_ID}/${CMAKE_BUILD_TYPE}/$<TARGET_FILE_NAME:pruv_http_worker>
    VERBATIM
)

add_custom_command(TARGET ${PROJECT_NAME}_sample_handlers POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pruv_http_sample_handlers>
        ${PROJECT_SOURCE_DIR}/bin/${CMAKE_CXX_COMPILER_ID}/${CMAKE_BUILD_TYPE}/$<TARGET_FILE_NAME:pruv_http_sample_handlers>
    VERBATIM
)
