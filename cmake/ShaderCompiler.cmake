find_program(DXC_EXE dxc
    HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "C:/Program Files (x86)/Windows Kits/10/bin/*/x64"
    DOC "Path to the DirectX Shader Compiler (dxc.exe)"
)

if(NOT DXC_EXE)
    message(FATAL_ERROR "DXC not found!")
endif()

message(STATUS "DXC: ${DXC_EXE}")

function(compile_shader SHADER_FILE ENTRY_POINT PROFILE OUTPUT_HEADER)
    set(SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders/${SHADER_FILE}")
    set(SHADER_OUT "${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders/${OUTPUT_HEADER}")

    get_filename_component(HEADER_DIR "${SHADER_OUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${HEADER_DIR}")

    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DXC_DEBUG_FLAGS /Od /Zi)
    else()
        set(DXC_DEBUG_FLAGS /O3 /Qstrip_reflect)
    endif()

    set(SHADER_DEPS "${SHADER_SRC}")
    file(READ "${SHADER_SRC}" SHADER_CONTENT)
    string(REGEX MATCHALL "#include[ \t]*\"([^\"]+)\"" SHADER_INCLUDE_MATCHES "${SHADER_CONTENT}")
    foreach(MATCH IN LISTS SHADER_INCLUDE_MATCHES)
        string(REGEX REPLACE "#include[ \t]*\"([^\"]+)\"" "\\1" SHADER_INCLUDE_FILE "${MATCH}")
        list(APPEND SHADER_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders/${SHADER_INCLUDE_FILE}")
    endforeach()

    add_custom_command(
        OUTPUT "${SHADER_OUT}"
        COMMAND "${DXC_EXE}" "${SHADER_SRC}" /E "${ENTRY_POINT}" /T "${PROFILE}"
                /Fh "${SHADER_OUT}" ${DXC_DEBUG_FLAGS} /WX
        DEPENDS ${SHADER_DEPS}
        COMMENT "Compiling ${PROFILE}: ${SHADER_FILE} (${ENTRY_POINT})"
    )
endfunction()
