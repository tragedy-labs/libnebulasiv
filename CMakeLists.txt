cmake_minimum_required(VERSION 3.13)
project(libnebulasiv C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Emit build/compile_commands.json so editors/clangd know the include paths.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra)

# Capability modules + transport. All command logic lives here; per-chip
# bundle headers (include/um98x.h) only aggregate these.
add_library(nebulasiv STATIC
  src/serial.c
  src/neb_serial_transport.c
  src/neb_core.c
  src/neb_mode.c
  src/neb_config.c
  src/neb_rtk.c
  src/neb_mask.c
  src/neb_assist.c
  src/neb_heading.c
  src/neb_admin.c
  src/neb_logging.c
)
target_include_directories(nebulasiv PUBLIC include)
