cmake_minimum_required(VERSION 3.16)

project(DTN_Edge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

# Exécutable principal (ajoute src/json_codec.cpp et src/network_manager.cpp dès que tu les crées)
add_executable(dtn_edge
    src/main.cpp
    src/data_acquisition.cpp
    src/message_manager.cpp
    src/storage.cpp
    transformation/src/to_http.cpp
    transformation/src/to_mqtt.cpp
)

target_include_directories(dtn_edge PRIVATE
    include
    transformation/include
)

target_link_libraries(dtn_edge PRIVATE
    SQLite::SQLite3
    Threads::Threads
    stdc++fs
)

# Exécutable temporaire pour tester le Module 3 (SQLite Storage) seul
add_executable(test_storage
    test_storage.cpp
    src/storage.cpp
)

target_include_directories(test_storage PRIVATE include)

target_link_libraries(test_storage PRIVATE
    SQLite::SQLite3
    Threads::Threads
    stdc++fs
)