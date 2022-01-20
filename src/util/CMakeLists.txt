add_library(util STATIC)
set_warnings_as_errors(util)
target_compile_features(util PUBLIC cxx_std_17)

target_include_directories(util PUBLIC .)

target_link_libraries(util PUBLIC stb)

target_sources(util PRIVATE
    image.cpp
    )
