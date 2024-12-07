cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CONAN_DISABLE_CHECK_COMPILER ON)


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
        src/main.cpp
        src/http_server.cpp
        src/model/map.cpp
        src/model/game.cpp
        src/model/player.cpp
        src/model/dog.cpp
        src/model/road.cpp
        src/model/office.cpp
        src/model/building.cpp
        src/model/game_session.cpp
        src/model/roadmap.cpp
        src/model/support_types.cpp
        src/json/json_loader.cpp
        src/json/json_converter.cpp
        src/request_handlers/request_handler.cpp
        src/utils/request_handlers_utils.cpp
        src/utils/filesystem_utils.cpp
        src/utils/random_generators.cpp
        src/boost_json.cpp
        src/logging/logging_data_storage.cpp
        src/logging/logger.cpp
        src/authentication/player_tokens.cpp
        src/app/application.cpp
)
target_include_directories(game_server PRIVATE
        CONAN_PKG::boost
        src
        src/json
        src/request_handlers
        src/utils
        src/logging
        src/model
        src/authentication
        src/app
)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)
# используем "импортированную" цель CONAN_PKG::boost
