add_executable(Playground WIN32)

target_sources(Playground PRIVATE
    "Source/Playground.cpp"
    "Source/Playground.hpp"
    "Source/main.cpp"
)

target_link_libraries(Playground PRIVATE GEngine)

# Copy sample assets to the build output directory
add_custom_command(TARGET Playground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:Playground>/Assets"
    COMMENT "Copying Playground assets to build directory..."
)

# Copy compiled shaders next to the executable
add_custom_target(PlaygroundShaders DEPENDS ${GENGINE_SHADERS})
add_custom_command(TARGET PlaygroundShaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Playground>/Assets/Shaders"
    COMMENT "Ensuring shader runtime directory exists"
)
foreach(shader IN LISTS GENGINE_SHADERS)
    get_filename_component(shader_name "${shader}" NAME)
    add_custom_command(TARGET PlaygroundShaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${shader}" "$<TARGET_FILE_DIR:Playground>/Assets/Shaders/${shader_name}"
        COMMENT "Copying shader ${shader_name} to runtime directory"
        VERBATIM
    )
endforeach()
add_dependencies(Playground PlaygroundShaders)
