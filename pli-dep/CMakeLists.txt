cmake_minimum_required(VERSION 3.24.2)
project(hw LANGUAGES PLI)

add_executable(hw hw.pli)
target_include_directories(hw PUBLIC ./)
