cmake_minimum_required(VERSION 3.26)
project(img2sdf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DDEBUG /D_DEBUG")

add_subdirectory(src)


add_compile_definitions("NOMINMAX") #deal with collisions with std::min, std::max
add_compile_options("/fsanitize=address")
add_link_options("/fsanitize=address")
LIST(APPEND CMAKE_PROGRAM_PATH  "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.37.32822\\bin\\Hostx64\\x64" ...)


include(FetchContent)
FetchContent_Declare(argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git)
FetchContent_MakeAvailable(argparse)

include_directories(${argparse_SOURCE_DIR}/include)


add_dependencies(${PROJECT_NAME} shaders)

target_link_libraries(${PROJECT_NAME}
        PRIVATE d3d11.lib d3dcompiler.dll dxguid.lib argparse windowscodecs.lib runtimeobject.lib
        clang_rt.asan_dynamic-x86_64.dll clang_rt.asan_dbg_dynamic-x86_64.dll )

