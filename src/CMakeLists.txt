add_library(nf MODULE
        main.cpp
        plugin.cpp
        alias_manager.cpp
        plugin_context.cpp
        error_logger.cpp
        error.cpp
        paths.cpp
        common.cpp
)

target_include_directories(nf SYSTEM PRIVATE
        "${FAR2L_SDK_DIR}"
        "${FAR2L_WINPORT_DIR}"
)
target_include_directories(nf PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_options(nf PRIVATE -fvisibility=default)

# Обязательно для внешних плагинов: запрещает линковаться к приватным символам far2l.
target_compile_definitions(nf PRIVATE -DFAR_DONT_USE_INTERNALS)

set_target_properties(nf PROPERTIES
        PREFIX ""
        OUTPUT_NAME "nf"
        SUFFIX ".far-plug-wide"
)

add_custom_command(TARGET nf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/configs"
        "$<TARGET_FILE_DIR:nf>"
        COMMENT "Copying nf language files next to the plugin binary"
)