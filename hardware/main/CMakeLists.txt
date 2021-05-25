idf_component_register(
    SRCS "filesystem.c" "list.c" "main.c" "metering.c" "wifi.c" "webserver.c"
    INCLUDE_DIRS ""
)

file(MAKE_DIRECTORY ../spiffs_image)
file(
    COPY

    ${CMAKE_SOURCE_DIR}/../software/chart.min.js
    ${CMAKE_SOURCE_DIR}/../software/index.html
    ${CMAKE_SOURCE_DIR}/../software/languages.js
    ${CMAKE_SOURCE_DIR}/../software/main.js
    ${CMAKE_SOURCE_DIR}/../software/manual.js
    ${CMAKE_SOURCE_DIR}/../software/points.js
    ${CMAKE_SOURCE_DIR}/../software/settings.js
    ${CMAKE_SOURCE_DIR}/../software/style.css

    DESTINATION ${CMAKE_SOURCE_DIR}/spiffs_image
)

spiffs_create_partition_image(
    storage
    ../spiffs_image
    FLASH_IN_PROJECT
)