add_executable(CubiEngine "Main.cpp")

target_include_directories(CubiEngine PUBLIC "Include" "../Shaders")
target_link_libraries(CubiEngine PUBLIC External)

# Enable hot reload in Visual studio 2022.
if (MSVC AND WIN32 AND NOT MSVC_VERSION VERSION_LESS 142)
    target_link_options(CubiEngine PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL>)
    target_compile_options(CubiEngine PRIVATE $<$<CONFIG:Debug>:/Zi>)
endif()