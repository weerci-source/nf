add_library(nf SHARED
        Plugin.cpp
        main.cpp
)

target_include_directories(nf SYSTEM PRIVATE
        "${FAR2L_SDK_DIR}"
        "${FAR2L_WINPORT_DIR}"
)
target_include_directories(nf PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
)

# Для far2l плагины обычно собираются с видимыми символами.
target_compile_options(nf PRIVATE -fvisibility=default)

set_target_properties(nf PROPERTIES
        PREFIX ""
        OUTPUT_NAME "nf"
        SUFFIX ".far-plug-wide"
)

