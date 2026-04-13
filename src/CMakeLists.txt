cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS $ENV{HOME}/Developer/VMUPro-SDK/c)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Determines the final filename like /build/nofrendo.app.elf
project(smsplus)

include(elf_loader)
project_elf(smsplus_elf)

target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE -Wl,--unresolved-symbols=ignore-all)