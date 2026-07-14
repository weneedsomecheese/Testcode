cmake_minimum_required(VERSION 3.10)
project(il2cpp_mod)

set(CMAKE_CXX_STANDARD 17)

add_library(modmenu SHARED
    jni/src/main.cpp
    jni/src/il2cpp.cpp
    jni/src/hooks.cpp
    jni/src/menu.cpp
    jni/src/thumbhook.cpp
)

target_include_directories(modmenu PRIVATE jni/include)
target_compile_options(modmenu PRIVATE -O2 -fvisibility=hidden -fno-rtti -fno-exceptions)
target_link_libraries(modmenu log android)
