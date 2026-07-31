add_executable(Playground WIN32)

target_sources(Playground PRIVATE
    "Source/Playground.cpp"
    "Source/Playground.hpp"
    "Source/main.cpp"
)

target_link_libraries(Playground PRIVATE GEngine)
