cmake_minimum_required(VERSION 3.20)
project(Threads)

set(CMAKE_CXX_STANDARD 14)

add_executable(Threads main.cpp job_system.h Thread.h Thread.cpp primitives.h)
