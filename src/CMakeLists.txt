#Build HLSL
add_custom_target(shaders COMMENT "Shader Compilation")

set(HLSL_SHADER_FILES jumpflood.hlsl, preprocess.hlsl)
set_source_files_properties(jumpflood.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(jumpflood.hlsl PROPERTIES EntryPoint "main")

set_source_files_properties(preprocess.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(preprocess.hlsl PROPERTIES EntryPoint "preprocess")

set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "5_0")


foreach(FILE ${HLSL_SHADER_FILES})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    get_source_file_property(shadertype ${FILE} ShaderType)
    get_source_file_property(shadermodel ${FILE} ShaderModel)
    get_source_file_property(entrypoint ${FILE} EntryPoint)
    add_custom_command(TARGET shaders
            COMMAND fxc.exe /nologo /E${entrypoint} /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fh ${CMAKE_SOURCE_DIR}/shaders/${FILE_WE}.hcs /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${CMAKE_SOURCE_DIR}/shaders/${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            VERBATIM)
endforeach(FILE)

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
        JumpFloodResources.cpp
        JumpFloodResources.h
)

