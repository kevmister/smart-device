# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS
  "main.c" 
  "programs.c"
  "programs/program_air_suspension.c"
  "services.c"
  "services/service_canbus.c"
  "services/service_http_server.c"
  "services/service_mdns.c"
  "services/service_mpu6050.c"
  "services/service_ssd1315.c"
  "services/service_wifi.c"
)
set(COMPONENT_ADD_INCLUDEDIRS "")
set(AUX_SOURCE_DIRECTORY "")

register_component()

# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'. 
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
