# u8g2
set(U8G2_SDK_PATH "${CMAKE_SOURCE_DIR}/u8g2")

# include u8g2
aux_source_directory("${U8G2_SDK_PATH}/csrc" SRC_LIST)
include_directories("${U8G2_SDK_PATH}/csrc")
