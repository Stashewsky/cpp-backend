cmake_minimum_required(VERSION 3.15)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

include(CTest)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake) 
else()
    include(${CONAN_BUILD_DIRS_CATCH2_RELEASE}/Catch.cmake) 
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library (game_server_lib STATIC
	src/boost_json.cpp
	src/tagged.h
	src/sdk.h
	src/magic_defs.h
	src/model.h
	src/model.cpp
	src/model_serialization.h
	src/json_loader.h
	src/json_loader.cpp
	src/tools.cpp
	src/tools.h
	src/logger.cpp
	src/logger.h
	src/application.cpp
	src/application.h
	src/program_options.cpp
	src/program_options.h
	src/loot_generator.cpp
	src/loot_generator.h
	src/extra_data.cpp
	src/extra_data.h
	src/collision_detector.cpp
	src/collision_detector.h
	src/geom.cpp
	src/geom.h
	src/postgres.cpp
	src/postgres.h
	src/records_player.cpp
	src/records_player.h
	src/tagged_uuid.cpp
	src/tagged_uuid.h
)
target_include_directories(game_server_lib PUBLIC  CONAN_PKG::boost)
target_link_libraries(game_server_lib PUBLIC CONAN_PKG::boost Threads::Threads)
target_link_libraries(game_server_lib PUBLIC CONAN_PKG::libpq CONAN_PKG::libpqxx)


add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/api_request.cpp
	src/api_request.h
	src/request_handler.cpp
	src/request_handler.h
	src/response.cpp
	src/response.h
	src/token.h
	src/token.cpp
	src/uri_api.h
	src/static_request.cpp
	src/static_request.h
)
target_link_libraries(game_server PRIVATE game_server_lib)


add_executable(game_server_tests
    	tests/loot_generator_tests.cpp
		tests/collision_detector_tests.cpp
		#tests/state-serialization-tests.cpp
)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 game_server_lib)

catch_discover_tests(game_server_tests) 





