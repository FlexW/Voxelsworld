add_library(log STATIC)
set_warnings_as_errors(log)
target_compile_features(log PUBLIC cxx_std_17)

target_include_directories(log PUBLIC .)

target_sources(log PRIVATE
    log.cpp)
