
add_subdirectory(tools)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DDEBUG /D_DEBUG")

add_compile_definitions("NOMINMAX") #deal with collisions with std::min, std::max

LIST(APPEND CMAKE_PROGRAM_PATH  "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.37.32822\\bin\\Hostx64\\x64" ...)


#Build HLSL
add_custom_target(shaders COMMENT "Shader Compilation")

set(HLSL_SHADER_FILES jumpflood.hlsl preprocess.hlsl distance.hlsl voronoi_normalise.hlsl
        minmax_reduce.hlsl minmaxreduce_firstpass.hlsl normalise.hlsl)
set_source_files_properties(jumpflood.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(jumpflood.hlsl PROPERTIES EntryPoint "main")

set_source_files_properties(preprocess.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(preprocess.hlsl PROPERTIES EntryPoint "preprocess")

set_source_files_properties(distance.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(distance.hlsl PROPERTIES EntryPoint "distance")

set_source_files_properties(voronoi_normalise.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(voronoi_normalise.hlsl PROPERTIES EntryPoint "voronoi_normalise")

set_source_files_properties(minmax_reduce.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(minmax_reduce.hlsl PROPERTIES EntryPoint "reduce")

set_source_files_properties(minmaxreduce_firstpass.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(minmaxreduce_firstpass.hlsl PROPERTIES EntryPoint "reduce_firstpass")

set_source_files_properties(normalise.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(normalise.hlsl PROPERTIES EntryPoint "normalise")

set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "5_0")


foreach(FILE ${HLSL_SHADER_FILES})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    get_source_file_property(shadertype ${FILE} ShaderType)
    get_source_file_property(shadermodel ${FILE} ShaderModel)
    get_source_file_property(entrypoint ${FILE} EntryPoint)
    add_custom_command(TARGET shaders
            COMMAND fxc.exe /nologo /E${entrypoint} /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fh ${CMAKE_SOURCE_DIR}/src/shaders/${FILE_WE}.hcs /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${CMAKE_SOURCE_DIR}/src/shaders/${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            VERBATIM)
endforeach(FILE)


#add_executable(img2sdf tools/program.cpp
#        dxinit.cpp
#        dxinit.h
#        WICTextureLoader.cpp
#        WICTextureLoader.h
#        dxutils.cpp
#        dxutils.h
#        shader_globals.h
#        program.h
#        WICTextureWriter.cpp
#        WICTextureWriter.h
#        JumpFloodResources.cpp
#        JumpFloodResources.h
#        JumpFloodDispatch.cpp
#        JumpFloodDispatch.h
#        jumpflooderror.cpp
#        jumpflooderror.h
#        img2sdf.cpp
#        img2sdf.h
#
#)

#target_compile_options(img2sdf PUBLIC "/fsanitize=address")
#target_link_options(img2sdf PUBLIC "/fsanitize=address")

add_library(libimg2sdf STATIC
        dxinit.cpp
        dxinit.h
        WICTextureLoader.cpp
        WICTextureLoader.h
        dxutils.cpp
        dxutils.h
        shader_globals.h
        WICTextureWriter.cpp
        WICTextureWriter.h
        JumpFloodResources.cpp
        JumpFloodResources.h
        JumpFloodDispatch.cpp
        JumpFloodDispatch.h
        jumpflooderror.cpp
        jumpflooderror.h
        img2sdf.cpp
        img2sdf.h)


target_link_libraries(libimg2sdf PRIVATE d3d11.lib d3dcompiler.dll dxguid.lib argparse windowscodecs.lib runtimeobject.lib)