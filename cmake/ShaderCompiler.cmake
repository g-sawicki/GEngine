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
# Compiles one HLSL entry point to a DXIL container (.cso)
function(compile_shader SHADER_FILE ENTRY_POINT PROFILE OUTPUT_CSO)
    set(SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders/${SHADER_FILE}")
    set(SHADER_OUT "${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders/${OUTPUT_CSO}")
    set(SHADER_DEP_FILE "${SHADER_OUT}.d")

    get_filename_component(SHADER_OUT_DIR "${SHADER_OUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${SHADER_OUT_DIR}")

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
        # Compile the shader.
        COMMAND "${DXC_EXE}" "${SHADER_SRC}" /E "${ENTRY_POINT}" /T "${PROFILE}"
                /Fo "${SHADER_OUT}" ${SHADER_DEBUG_FLAGS} ${SHADER_RELEASE_FLAGS} /WX
        # Emit the dependency file
        COMMAND "${DXC_EXE}" "${SHADER_SRC}" /E "${ENTRY_POINT}" /T "${PROFILE}" /M /MF "${SHADER_DEP_FILE}"
        DEPENDS "${SHADER_SRC}"
        DEPFILE "${SHADER_DEP_FILE}"
        COMMENT "Compiling ${PROFILE}: ${SHADER_FILE} (${ENTRY_POINT})"
        VERBATIM
    )
endfunction()
