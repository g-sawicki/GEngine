add_executable(Playground WIN32)

target_sources(Playground PRIVATE
    "Playground.cpp"
    "Playground.hpp"
    "main.cpp"
)

target_link_libraries(Playground PRIVATE GEngine)
