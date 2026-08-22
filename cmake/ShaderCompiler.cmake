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

# compile_shader(<shader.hlsl> <entry_point> <profile> <output.cso>)
#
# Compiles one HLSL entry point to a DXIL container (.cso) with DXC.
#
# Produces: ${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders/<output.cso>
#
# Per-config flags use generator expressions so this works for both single-config
# (Ninja) and multi-config (Visual Studio) generators (CMAKE_BUILD_TYPE is empty
# for multi-config generators).
function(compile_shader SHADER_FILE ENTRY_POINT PROFILE OUTPUT_CSO)
    set(SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders/${SHADER_FILE}")
    set(SHADER_OUT "${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders/${OUTPUT_CSO}")

    get_filename_component(SHADER_OUT_DIR "${SHADER_OUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${SHADER_OUT_DIR}")

    # Rebuild when the shader or anything it #includes changes.
    set(SHADER_DEPS "${SHADER_SRC}")
    file(READ "${SHADER_SRC}" SHADER_CONTENT)
    string(REGEX MATCHALL "#include[ \t]*\"([^\"]+)\"" SHADER_INCLUDE_MATCHES "${SHADER_CONTENT}")
    foreach(MATCH IN LISTS SHADER_INCLUDE_MATCHES)
        string(REGEX REPLACE "#include[ \t]*\"([^\"]+)\"" "\\1" SHADER_INCLUDE_FILE "${MATCH}")
        list(APPEND SHADER_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders/${SHADER_INCLUDE_FILE}")
    endforeach()

    # Debug: keep shader debug info (for PIX/GPU debugging) and disable optimization.
    # Other: optimize and strip reflection data. One generator expression per flag so
    # the value list is preserved for both single- and multi-config generators.
    set(SHADER_DEBUG_FLAGS
        "$<$<CONFIG:Debug>:/Zi>"
        "$<$<CONFIG:Debug>:/Qembed_debug>"
        "$<$<CONFIG:Debug>:/Od>"
    )
    set(SHADER_RELEASE_FLAGS
        "$<$<NOT:$<CONFIG:Debug>>:/O3>"
        "$<$<NOT:$<CONFIG:Debug>>:/Qstrip_reflect>"
    )

    add_custom_command(
        OUTPUT "${SHADER_OUT}"
        COMMAND "${DXC_EXE}" "${SHADER_SRC}" /E "${ENTRY_POINT}" /T "${PROFILE}"
                /Fo "${SHADER_OUT}" ${SHADER_DEBUG_FLAGS} ${SHADER_RELEASE_FLAGS} /WX
        DEPENDS ${SHADER_DEPS}
        COMMENT "Compiling ${PROFILE}: ${SHADER_FILE} (${ENTRY_POINT})"
        VERBATIM
    )
endfunction()
