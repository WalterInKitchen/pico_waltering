include_directories(inc)

aux_source_directory(. SRC_LIST)
aux_source_directory(src SRC_LIST)

add_executable(watering ${SRC_LIST})

# pull in common dependencies
target_link_libraries(watering pico_stdlib)

# enable usb output, disable uart output
pico_enable_stdio_usb(watering 1)
pico_enable_stdio_uart(watering 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(watering)