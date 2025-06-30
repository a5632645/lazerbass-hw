#THIS FILE IS AUTO GENERATED FROM THE TEMPLATE! DO NOT CHANGE!
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.26)

#########################################
# Environment
#########################################
set(OPENOCD_BIN_PATH       "openocd")
set(OPENOCD_INTERFACE_FILE "${CMAKE_SOURCE_DIR}/cmsis-dap.cfg")
set(OPENOCD_TARGET_FILE    "${CMAKE_SOURCE_DIR}/stm32h7x.cfg")
set(OPENOCD_FLASH_BEGIN    0x08000000)
set(HARD_FPU_ON            TRUE)
set(PRJ_NAME               "Lazerbass")

#########################################
# 工具栏设置
#########################################
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#########################################
# project settings
#########################################
project(${PRJ_NAME} C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

# 浮点数设定
if(HARD_FPU_ON)
    add_compile_definitions(ARM_MATH_CM7;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
    add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
    add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
else()
    add_compile_options(-mfloat-abi=soft)
endif()

#########################################
# MCU架构
#########################################
add_compile_options(-mcpu=cortex-m7 -mthumb)
add_compile_options(
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -fno-exceptions
    -Wextra
)

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)
set(CMAKE_BUILD_TYPE "Debug")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

include("usflib/include/single_header.cmake")

#########################################
# RTOS
#########################################
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM
    INTERFACE "Lazerbass/lib_config")
set(FREERTOS_PORT GCC_ARM_CM7 CACHE STRING "")
set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
add_subdirectory("rtos")

#########################################
# project files
#########################################
include_directories(
    "stm32h7xx-hal-driver/Inc"
    "CMSIS_5/CMSIS/Core/include"
    
    "Lazerbass/lib_config"
    "Lazerbass/mcu_hal"
    "Lazerbass"

    "STM32_USB_Device_Library/Core/Inc"
    "Lazerbass/bsp/st_usb"
    "usflib/include"
)
file(GLOB_RECURSE SOURCES
    "stm32h7xx-hal-driver/Src/*.c"
    "CMSIS_5/CMSIS/Core/Source/*.c"

    "Lazerbass/*.c"
    "Lazerbass/*.cpp"
    "Lazerbass/*.s"

    "STM32_USB_Device_Library/Core/Src/*.c"
    "Lazerbass/bsp/st_usb/*.c"
)

add_compile_definitions(
    DEBUG
    USE_HAL_DRIVER
    STM32H750xx
    DATA_IN_D2_SRAM
    __PROGRAM_START
    USE_USB_FS
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32H750XBHX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})
target_link_libraries(${PROJECT_NAME}.elf freertos_config freertos_kernel)

########################################
# 生成hex和bin文件
#########################################
set(ELF_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf)
set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")

# openocd下载
add_custom_target(Download
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${OPENOCD_BIN_PATH}
    -f ${OPENOCD_INTERFACE_FILE}
    -f ${OPENOCD_TARGET_FILE}
    -c "init"
    -c "halt"
    -c "halt"
    -c "halt"
    -c "halt"
    -c "halt"
    -c "flash write_image erase build/${PROJECT_NAME}.bin ${OPENOCD_FLASH_BEGIN}"
    -c "reset"
    -c "shutdown"
)

# openocd重置
add_custom_target(Reset
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${OPENOCD_BIN_PATH}
    -f ${OPENOCD_INTERFACE_FILE}
    -f ${OPENOCD_TARGET_FILE}
    -c "init"
    -c "halt"
    -c "reset"
    -c "shutdown"
)

# 暂停
add_custom_target(Pause
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${OPENOCD_BIN_PATH}
    -f ${OPENOCD_INTERFACE_FILE}
    -f ${OPENOCD_TARGET_FILE}
    -c "init"
    -c "halt"
    -c "shutdown"
)

# 运行
add_custom_target(Resume
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${OPENOCD_BIN_PATH}
    -f ${OPENOCD_INTERFACE_FILE}
    -f ${OPENOCD_TARGET_FILE}
    -c "init"
    -c "resume"
    -c "shutdown"
)

# openocd
add_custom_target(OpenOCD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${OPENOCD_BIN_PATH}
    -f ${OPENOCD_INTERFACE_FILE}
    -f ${OPENOCD_TARGET_FILE}
    -c "init"
    -c "halt"
)