cmake_minimum_required(VERSION 3.5)
project(app_service CXX)

include(CheckIncludeFileCXX)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
if (NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
    # Do nothing
elseif (HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
    set(CMAKE_CXX_STANDARD 20)
elseif (HAS_ANY AND HAS_STRING_VIEW)
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 14)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} main.cpp)

# Dependencies (System installed)
find_package(Drogon CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

find_package(jwt-cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE jwt-cpp::jwt-cpp)

if (CMAKE_CXX_STANDARD LESS 17)
    find_package(Boost 1.61.0 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)
endif ()

# Source directories
aux_source_directory(controllers CTL_SRC)
aux_source_directory(controllers/UserController CTL_SRC_USER)
aux_source_directory(controllers/PostController CTL_SRC_POST)
aux_source_directory(controllers/CommentController CTL_SRC_COMMENT)
aux_source_directory(controllers/FeedController CTL_SRC_FEED)
aux_source_directory(controllers/MediaController CTL_SRC_MEDIA)
aux_source_directory(filters FILTER_SRC)
aux_source_directory(models MODEL_SRC)

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_CURRENT_SOURCE_DIR}/models)

target_sources(${PROJECT_NAME}
               PRIVATE
               ${CTL_SRC}
               ${CTL_SRC_USER}
               ${CTL_SRC_POST}
               ${CTL_SRC_COMMENT}
               ${CTL_SRC_FEED}
               ${CTL_SRC_MEDIA}
               ${FILTER_SRC}
               ${MODEL_SRC})
