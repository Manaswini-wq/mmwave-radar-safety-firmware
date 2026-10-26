cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# AWR2944 mmWave Radar Safety Firmware
# ISO 26262 ASIL-D / IEC 61508 SIL-3
# ==============================================================================

project(awr2944_fw
    VERSION 1.0.0
    DESCRIPTION "Safety-critical mmWave radar firmware for TI AWR2944"
    LANGUAGES C ASM
)

# Target CPU
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain (set via CMAKE_TOOLCHAIN_FILE or defaults)
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER arm-none-eabi-gcc)
endif()
if(NOT CMAKE_ASM_COMPILER)
    set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
endif()

# CPU flags
set(CPU_FLAGS "-mcpu=cortex-r5 -mthumb -mfloat-abi=hard -mfpu=vfpv3-d16")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPU_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${CPU_FLAGS}")

# C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler warnings (MISRA-C style strictness)
add_compile_options(
    -Wall -Wextra -Werror
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wconversion
    -Wshadow
    -ffunction-sections
    -fdata-sections
)

# Definitions
add_definitions(
    -DTARGET_AWR2944
    -DSAFETY_BUILD=1
    -DSAFETY_CHECKS_ENABLED=1
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/safety
    ${CMAKE_SOURCE_DIR}/src/radar
    ${CMAKE_SOURCE_DIR}/src/calibration
    ${CMAKE_SOURCE_DIR}/src/drivers
    ${CMAKE_SOURCE_DIR}/src/rom
    ${CMAKE_SOURCE_DIR}/src/hal
)

# ==============================================================================
# Source files
# ==============================================================================

set(C_SOURCES
    src/main.c
    src/startup/startup_r5f.c
    src/safety/safety_monitor.c
    src/safety/fault_handler.c
    src/safety/diagnostic_coverage.c
    src/safety/watchdog.c
    src/safety/ecc_handler.c
    src/safety/lockstep_monitor.c
    src/safety/crc_e2e.c
    src/safety/control_flow_monitor.c
    src/radar/radar_control.c
    src/radar/chirp_config.c
    src/radar/data_acquisition.c
    src/calibration/tx_power_cal.c
    src/calibration/rx_gain_phase_cal.c
    src/calibration/lo_leakage_cal.c
    src/drivers/adc_driver.c
    src/drivers/dac_driver.c
    src/drivers/pll_driver.c
    src/drivers/spi_driver.c
    src/drivers/gpio_driver.c
    src/rom/rom_patch.c
    src/rom/trim_loader.c
    src/hal/hal_interrupt.c
    src/hal/hal_timer.c
    src/hal/hal_dma.c
)

set(ASM_SOURCES
    src/startup/vector_table.S
    asm/critical_section.S
    asm/cache_ops.S
)

# ==============================================================================
# Main firmware target
# ==============================================================================

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/awr2944_r5f.lds)

add_executable(${PROJECT_NAME}.elf ${C_SOURCES} ${ASM_SOURCES})

set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    LINK_FLAGS "-T ${LINKER_SCRIPT} -Wl,--gc-sections -Wl,-Map=${PROJECT_NAME}.map -nostartfiles"
)

target_link_libraries(${PROJECT_NAME}.elf m c gcc)

# Generate binary and hex files
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Generating binary and hex output files"
)

# ==============================================================================
# Test targets (host-compiled unit tests)
# ==============================================================================

option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    # Override compiler for host tests
    # Tests run on the host, not on the target
    add_executable(test_safety_monitor
        tests/unit/test_safety_monitor.c
        src/safety/safety_monitor.c
        src/safety/crc_e2e.c
        src/safety/control_flow_monitor.c
    )
    target_link_libraries(test_safety_monitor unity)
    add_test(NAME SafetyMonitor COMMAND test_safety_monitor)

    add_executable(test_crc_e2e
        tests/unit/test_crc_e2e.c
        src/safety/crc_e2e.c
    )
    target_link_libraries(test_crc_e2e unity)
    add_test(NAME CRC_E2E COMMAND test_crc_e2e)
endif()
