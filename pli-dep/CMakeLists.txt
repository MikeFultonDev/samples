cmake_minimum_required(VERSION 3.24.2)
project(hw LANGUAGES PLI)

include_directories(include)

set(hwsrc src/hw.pli src/fn.pli)

add_executable(hw ${hwsrc})
