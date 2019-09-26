cmake_minimum_required(VERSION 3.14)
project(RxSimex)

find_package(Matlab REQUIRED)
include(cmake_utils.cmake)

set(CMAKE_CXX_STANDARD 17)
set(Simulink_DIR ${Matlab_ROOT_DIR}/simulink)

include_directories(include)
include_directories(include/RxCpp/Rx/v2/src/rxcpp)
include_directories(include/xtensor/include)
include_directories(include/xtl/include)
include_directories(include/fmt/include)
include_directories(${Matlab_INCLUDE_DIRS})
include_directories(${Simulink_DIR}/include)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(S_FUNCTION_LEVEL=2)
if (WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif ()

file(GLOB_RECURSE RXSIMEX_SOURCES src/*.c src/*.cpp)
file(GLOB FMT_SOURCES include/fmt/src/*.cc)

set(_SOURCES ${RXSIMEX_SOURCES} ${FMT_SOURCES})

if (${ENABLE_EXAMPLES})
    file(GLOB EXAMPLE_DIRS LIST_DIRECTORIES true examples/*)
    foreach (EXAMPLE_DIR ${EXAMPLE_DIRS})
        if (IS_DIRECTORY ${EXAMPLE_DIR})
            message(Adding example: ${EXAMPLE_DIR})
            add_subdirectory(${EXAMPLE_DIR})
        endif ()
    endforeach ()
endif ()