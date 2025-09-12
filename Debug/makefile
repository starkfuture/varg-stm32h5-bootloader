################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include sf_hal_stm32h5/timer_hal/subdir.mk
-include sf_hal_stm32h5/gpio_hal/subdir.mk
-include sf_hal_stm32h5/flash_hal/subdir.mk
-include sf_hal_stm32h5/crc_hal/subdir.mk
-include sf_hal_stm32h5/can_hal/subdir.mk
-include sf_hal_stm32h5/bootloader_hal/subdir.mk
-include services/memory/subdir.mk
-include services/fw-utils/tinycrypt/lib/source/subdir.mk
-include services/fw-utils/packet2/subdir.mk
-include services/fw-utils/nand_flash/subdir.mk
-include services/fw-utils/data_comm/subdir.mk
-include services/fw-utils/btea/subdir.mk
-include services/fw-utils/bootloader/fw_verification/subdir.mk
-include services/fw-utils/bootloader/subdir.mk
-include services/fw-utils/subdir.mk
-include services/can/subdir.mk
-include Drivers/STM32H5xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := varg-stm32h5-bootloader
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
varg-stm32h5-bootloader.elf \

MAP_FILES += \
varg-stm32h5-bootloader.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
varg-stm32h5-bootloader.list \

OBJCOPY_HEX += \
varg-stm32h5-bootloader.hex \

OBJCOPY_BIN += \
varg-stm32h5-bootloader.bin \


# All Target
all: main-build

# Main-build Target
main-build: varg-stm32h5-bootloader.elf secondary-outputs

# Tool invocations
varg-stm32h5-bootloader.elf varg-stm32h5-bootloader.map: $(OBJS) $(USER_OBJS) F:\home\marc-ros\STM32CubeIDE\workspace_1.18.1_new\varg-stm32h5-bootloader\STM32H563ZGTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "varg-stm32h5-bootloader.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m33 -T"F:\home\marc-ros\STM32CubeIDE\workspace_1.18.1_new\varg-stm32h5-bootloader\STM32H563ZGTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="varg-stm32h5-bootloader.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

varg-stm32h5-bootloader.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "varg-stm32h5-bootloader.list"
	@echo 'Finished building: $@'
	@echo ' '

varg-stm32h5-bootloader.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "varg-stm32h5-bootloader.hex"
	@echo 'Finished building: $@'
	@echo ' '

varg-stm32h5-bootloader.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "varg-stm32h5-bootloader.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) default.size.stdout varg-stm32h5-bootloader.bin varg-stm32h5-bootloader.elf varg-stm32h5-bootloader.hex varg-stm32h5-bootloader.list varg-stm32h5-bootloader.map
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
