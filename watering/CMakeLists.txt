include_directories(. inc common/inc hal/inc app/inc)

aux_source_directory(. SRC_LIST)
aux_source_directory(hal/src SRC_LIST)
aux_source_directory(common/src SRC_LIST)
aux_source_directory(app/src SRC_LIST)

add_executable(watering ${SRC_LIST})

# pull in common dependencies
target_link_libraries(watering pico_stdlib FreeRTOS-Kernel FreeRTOS-Kernel-Heap1)

# enable usb output, disable uart output
pico_enable_stdio_usb(watering 1)
pico_enable_stdio_uart(watering 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(watering)