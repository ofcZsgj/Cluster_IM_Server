# 定义一个SRC_LIST变量，包含该目录下的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./dao DB_LIST)
aux_source_directory(./modules MODULE_LIST)

# 指定生成可执行文件文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODULE_LIST})

# 指定生成可执行文件链接需要依赖的库
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread)
