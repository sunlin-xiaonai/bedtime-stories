idf_component_register(SRCS "main.c" "story.c" "sd_probe.c" "catalog.c" "player.c" "../prototype/sleep_timer.c"
    INCLUDE_DIRS "." "../prototype"
    REQUIRES yaykit_board lvgl__lvgl yaykit_app yaykit_key yaykit_link yaykit_ui yaykit_speak
    PRIV_REQUIRES nvs_flash console esp_timer fatfs sdmmc esp_driver_sdmmc esp_driver_usb_serial_jtag mbedtls
    EMBED_FILES "audio/khalai.bin" "audio/read.bin" "audio/counting.bin" "art/plant.bin" "art/book.bin" "art/animal.bin" "art/train.bin" "art/cat.bin"
    "art/plant_thumb.bin" "art/book_thumb.bin" "art/animal_thumb.bin" "art/train_thumb.bin" "art/cat_thumb.bin")
idf_component_get_property(board_dir yaykit_board COMPONENT_DIR)
esptool_py_flash_to_partition(flash "fonts" "${board_dir}/fonts/cjk/yaykit_fonts.bin")
target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_VERSION="${PROJECT_VER}")
