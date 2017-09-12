cmake_minimum_required(VERSION 2.8)

project(logging C)

set(LOGGING_SRC
  logging.c)
add_compile_options(-fPIC)
add_library(logging STATIC ${LOGGING_SRC})
