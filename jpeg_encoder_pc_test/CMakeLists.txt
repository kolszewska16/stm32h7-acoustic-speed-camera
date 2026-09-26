cmake_minimum_required(VERSION 3.10)
project(jpeg_encoder_pc_test)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(encode_jpeg
	main.c
	jpeg_encoder
)

target_include_directories(encode_jpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
