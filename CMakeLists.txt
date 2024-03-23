cmake_minimum_required(VERSION 3.26)
project(img2sdf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DDEBUG /D_DEBUG")



add_compile_definitions("NOMINMAX") #deal with collisions with std::min, std::max
add_compile_options("/fsanitize=address")
add_link_options("/fsanitize=address")
LIST(APPEND CMAKE_PROGRAM_PATH  "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.37.32822\\bin\\Hostx64\\x64" ...)


include(FetchContent)
FetchContent_Declare(argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git)
FetchContent_MakeAvailable(argparse)

include_directories(${argparse_SOURCE_DIR}/include)

add_executable(img2sdf library.cpp
        dxinit.cpp
        dxinit.h
        WICTextureLoader.cpp
        WICTextureLoader.h
        dxutils.cpp
        dxutils.h
        shader_globals.h
        library.h
        WICTextureWriter.cpp
        WICTextureWriter.h
)

#Build HLSL
add_custom_target(shaders COMMENT "Shader Compilation")

set(HLSL_SHADER_FILES preprocess.hlsl)
set_source_files_properties(preprocess.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(preprocess.hlsl PROPERTIES EntryPoint "preprocess")
#set_source_files_properties(jumpflood.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "5_0")


foreach(FILE ${HLSL_SHADER_FILES})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    get_source_file_property(shadertype ${FILE} ShaderType)
    get_source_file_property(shadermodel ${FILE} ShaderModel)
    get_source_file_property(entrypoint ${FILE} EntryPoint)
    add_custom_command(TARGET shaders
            COMMAND fxc.exe /nologo /E${entrypoint} /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fh ${CMAKE_SOURCE_DIR}/${FILE_WE}.hcs /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            VERBATIM)
endforeach(FILE)


add_dependencies(${PROJECT_NAME} shaders)

target_link_libraries(${PROJECT_NAME}
        PRIVATE d3d11.lib d3dcompiler.dll dxguid.lib argparse windowscodecs.lib runtimeobject.lib
        clang_rt.asan_dynamic-x86_64.dll clang_rt.asan_dbg_dynamic-x86_64.dll )

