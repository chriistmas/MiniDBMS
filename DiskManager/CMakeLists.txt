cmake_minimum_required(VERSION 3.10)
project(DiskManagerProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(disk_manager_test
    src/physical_disk.cpp
    src/page.cpp
    src/disk_manager.cpp
    src/record.cpp
    src/heap_file.cpp
    src/catalog.cpp
    src/csv_loader.cpp
    tests/main.cpp
)
