if(CMAKE_EXPORT_COMPILE_COMMANDS)
    # 定义一个来源路径（当前构建目录）和目标路径（项目根目录）
    set(SRC_JSON "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(DST_JSON "${CMAKE_SOURCE_DIR}/compile_commands.json")

    # 方法 A：在构建任何目标时，自动检查并更新根目录的 json (推荐)
    # 缺点：必须执行一次 build (哪怕是空 build) 才会同步
    add_custom_target(update_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC_JSON} ${DST_JSON}
        COMMENT "Updating compile_commands.json to root folder"
        VERBATIM
    )
endif()
