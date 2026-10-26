# ==============================================================================
# Makefile for AWR2944 mmWave Radar Safety Firmware
# ISO 26262 ASIL-D / IEC 61508 SIL-3
#
# Toolchain: TI ARM CGT (ti-cgt-arm) or GNU ARM (arm-none-eabi-gcc)
# Target:    Cortex-R5F (ARMv7-R, Thumb-2, VFPv3-D16)
# ==============================================================================

# Toolchain selection (set TI_CGT_PATH or use GNU ARM)
ifdef TI_CGT_PATH
  CC      = $(TI_CGT_PATH)/bin/armcl
  AS      = $(TI_CGT_PATH)/bin/armasm
  LD      = $(TI_CGT_PATH)/bin/armlnk
  OBJCOPY = $(TI_CGT_PATH)/bin/armobjcopy
  SIZE    = $(TI_CGT_PATH)/bin/armsize
else
  CROSS   ?= arm-none-eabi-
  CC      = $(CROSS)gcc
  AS      = $(CROSS)as
  LD      = $(CROSS)gcc
  OBJCOPY = $(CROSS)objcopy
  SIZE    = $(CROSS)size
endif

# Project
PROJECT  = awr2944_fw
BUILD_DIR = build

# Compiler flags
ARCH_FLAGS = -mcpu=cortex-r5 -mthumb -mfloat-abi=hard -mfpu=vfpv3-d16

CFLAGS  = $(ARCH_FLAGS)
CFLAGS += -Wall -Wextra -Werror -Wstrict-prototypes
CFLAGS += -O2 -g -gdwarf-3
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -std=c99
CFLAGS += -DTARGET_AWR2944 -DSAFETY_BUILD=1

# Include paths
INCLUDES  = -Iinclude
INCLUDES += -Isrc
INCLUDES += -Isrc/safety
INCLUDES += -Isrc/radar
INCLUDES += -Isrc/calibration
INCLUDES += -Isrc/drivers
INCLUDES += -Isrc/rom
INCLUDES += -Isrc/hal

CFLAGS += $(INCLUDES)

# Assembler flags
ASFLAGS = $(ARCH_FLAGS) -g

# Linker flags
LDFLAGS  = $(ARCH_FLAGS)
LDFLAGS += -T linker/awr2944_r5f.lds
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map
LDFLAGS += -nostartfiles
LDFLAGS += -lm -lc -lgcc

# Safety build flags (MISRA-C enforcement)
ifdef SAFETY_BUILD
CFLAGS += -DSAFETY_CHECKS_ENABLED=1
endif

# ==============================================================================
# Source files
# ==============================================================================

C_SRCS  = src/main.c
C_SRCS += src/startup/startup_r5f.c
C_SRCS += src/safety/safety_monitor.c
C_SRCS += src/safety/fault_handler.c
C_SRCS += src/safety/diagnostic_coverage.c
C_SRCS += src/safety/watchdog.c
C_SRCS += src/safety/ecc_handler.c
C_SRCS += src/safety/lockstep_monitor.c
C_SRCS += src/safety/crc_e2e.c
C_SRCS += src/safety/control_flow_monitor.c
C_SRCS += src/radar/radar_control.c
C_SRCS += src/radar/chirp_config.c
C_SRCS += src/radar/data_acquisition.c
C_SRCS += src/calibration/tx_power_cal.c
C_SRCS += src/calibration/rx_gain_phase_cal.c
C_SRCS += src/calibration/lo_leakage_cal.c
C_SRCS += src/drivers/adc_driver.c
C_SRCS += src/drivers/dac_driver.c
C_SRCS += src/drivers/pll_driver.c
C_SRCS += src/drivers/spi_driver.c
C_SRCS += src/drivers/gpio_driver.c
C_SRCS += src/rom/rom_patch.c
C_SRCS += src/rom/trim_loader.c
C_SRCS += src/hal/hal_interrupt.c
C_SRCS += src/hal/hal_timer.c
C_SRCS += src/hal/hal_dma.c

ASM_SRCS  = src/startup/vector_table.S
ASM_SRCS += asm/critical_section.S
ASM_SRCS += asm/cache_ops.S

# Object files
C_OBJS   = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
ASM_OBJS = $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SRCS))
ALL_OBJS = $(C_OBJS) $(ASM_OBJS)

# Dependency files
DEPS = $(C_OBJS:.o=.d)

# ==============================================================================
# Build targets
# ==============================================================================

.PHONY: all clean flash test-unit test-integration test-qualification test-safety test-calibration

all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin
	@echo "===== Build Complete ====="
	@$(SIZE) $(BUILD_DIR)/$(PROJECT).elf

$(BUILD_DIR)/$(PROJECT).elf: $(ALL_OBJS)
	@echo "[LD] $@"
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	@echo "[OBJCOPY] $@"
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(PROJECT).hex: $(BUILD_DIR)/$(PROJECT).elf
	@echo "[OBJCOPY] $@"
	$(OBJCOPY) -O ihex $< $@

# Compile C sources
$(BUILD_DIR)/%.o: %.c
	@echo "[CC] $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

# Assemble ASM sources
$(BUILD_DIR)/%.o: %.S
	@echo "[AS] $<"
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c -o $@ $<

# ==============================================================================
# Test targets
# ==============================================================================

test-unit:
	@echo "Running unit tests..."
	$(CC) -DUNIT_TEST $(CFLAGS) -o $(BUILD_DIR)/test_safety tests/unit/test_safety_monitor.c $(C_SRCS) -lunity
	$(BUILD_DIR)/test_safety
	$(CC) -DUNIT_TEST $(CFLAGS) -o $(BUILD_DIR)/test_crc tests/unit/test_crc_e2e.c $(C_SRCS) -lunity
	$(BUILD_DIR)/test_crc

test-safety: test-unit

test-calibration:
	@echo "Running calibration tests..."
	$(CC) -DUNIT_TEST $(CFLAGS) -o $(BUILD_DIR)/test_cal tests/unit/test_calibration.c $(C_SRCS) -lunity
	$(BUILD_DIR)/test_cal

test-integration:
	@echo "Running integration tests..."
	$(CC) -DUNIT_TEST $(CFLAGS) -o $(BUILD_DIR)/test_integ tests/integration/test_radar_data_path.c $(C_SRCS) -lunity
	$(BUILD_DIR)/test_integ

test-qualification:
	@echo "Running qualification tests..."
	$(CC) -DUNIT_TEST $(CFLAGS) -o $(BUILD_DIR)/test_qual tests/qualification/test_asil_d_requirements.c $(C_SRCS) -lunity
	$(BUILD_DIR)/test_qual

# ==============================================================================
# Flash target
# ==============================================================================

flash: $(BUILD_DIR)/$(PROJECT).bin
	python scripts/flash_jtag.py --interface xds200 --binary $<

# ==============================================================================
# Clean
# ==============================================================================

clean:
	rm -rf $(BUILD_DIR)

# Include dependency files
-include $(DEPS)
