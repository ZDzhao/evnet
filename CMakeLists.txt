project(evnet)
cmake_minimum_required(VERSION 2.8)

#设置库文件路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(deps/logging)

include_directories(deps/logging)

aux_source_directory(src/ SRC_LIST)

#创建头文件安装文件夹
execute_process(COMMAND mkdir -p ${PROJECT_BINARY_DIR}/include/${CMAKE_PROJECT_NAME}/ )
#遍历头文件
file(GLOB_RECURSE HEADER_FILES "${PROJECT_SOURCE_DIR}/src/*.h")
#拷贝头文件至安装文件夹
execute_process(COMMAND cp ${HEADER_FILES} ${PROJECT_BINARY_DIR}/include/${CMAKE_PROJECT_NAME}/ )

set(LINK_LIB_LIST)
list(APPEND  LINK_LIB_LIST event)
list(APPEND  LINK_LIB_LIST logging)

#打印库文件
message(STATUS "将链接依赖库:${LINK_LIB_LIST}")

#使能c++11
add_compile_options(-std=c++11)
add_compile_options(-Wno-deprecated-declarations)
add_compile_options(-Wno-predefined-identifier-outside-function)

#编译动态库
add_library(${CMAKE_PROJECT_NAME}_shared SHARED ${SRC_LIST})
target_link_libraries(${CMAKE_PROJECT_NAME}_shared ${LINK_LIB_LIST})
set_target_properties(${CMAKE_PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}")
install(TARGETS ${CMAKE_PROJECT_NAME}_shared LIBRARY DESTINATION lib)

#编译静态库
add_library(${CMAKE_PROJECT_NAME}_static STATIC ${SRC_LIST})
set_target_properties(${CMAKE_PROJECT_NAME}_static PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}")
install(TARGETS ${CMAKE_PROJECT_NAME}_static ARCHIVE DESTINATION lib)

#安装头文件至系统目录
install(DIRECTORY ${PROJECT_BINARY_DIR}/include/${CMAKE_PROJECT_NAME} DESTINATION include)

























