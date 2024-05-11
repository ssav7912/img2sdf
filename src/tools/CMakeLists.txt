set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DDEBUG /D_DEBUG")

add_compile_definitions("NOMINMAX") #deal with collisions with std::min, std::max


add_executable(img2sdf program.cpp)

target_link_libraries(img2sdf PUBLIC libimg2sdf)

target_link_libraries(img2sdf
        PRIVATE d3d11.lib d3dcompiler.dll dxguid.lib argparse windowscodecs.lib runtimeobject.lib
)

add_executable(profile profile.cpp ../Profiler.cpp)
target_link_libraries(profile PUBLIC libimg2sdf)
target_link_libraries(profile
        PRIVATE d3d11.lib d3dcompiler.dll dxguid.lib argparse windowscodecs.lib runtimeobject.lib
)
