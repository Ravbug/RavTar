cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ravtar)
add_library("${PROJECT_NAME}" "include/ravtar/tarball.hpp" "tarball.cpp")
target_include_directories("${PROJECT_NAME}" PUBLIC "include/" PRIVATE "include/ravtar")