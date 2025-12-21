add_library(glad STATIC
  include/glad/glad.h
  include/KHR/khrplatform.h
  src/glad.c
)

target_include_directories(glad PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
