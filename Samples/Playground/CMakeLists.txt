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

# Copy runtime DLLs next to the executable.
add_custom_command(TARGET Playground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        -t "$<TARGET_FILE_DIR:Playground>"
        $<TARGET_RUNTIME_DLLS:Playground>
    COMMENT "Copying runtime DLLs to build directory..."
)
