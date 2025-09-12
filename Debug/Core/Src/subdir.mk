################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/adc.c \
../Core/Src/crc.c \
../Core/Src/dac.c \
../Core/Src/fdcan.c \
../Core/Src/gpdma.c \
../Core/Src/gpio.c \
../Core/Src/icache.c \
../Core/Src/main.c \
../Core/Src/spi.c \
../Core/Src/stm32h5xx_hal_msp.c \
../Core/Src/stm32h5xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32h5xx.c \
../Core/Src/tim.c 

OBJS += \
./Core/Src/adc.o \
./Core/Src/crc.o \
./Core/Src/dac.o \
./Core/Src/fdcan.o \
./Core/Src/gpdma.o \
./Core/Src/gpio.o \
./Core/Src/icache.o \
./Core/Src/main.o \
./Core/Src/spi.o \
./Core/Src/stm32h5xx_hal_msp.o \
./Core/Src/stm32h5xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32h5xx.o \
./Core/Src/tim.o 

C_DEPS += \
./Core/Src/adc.d \
./Core/Src/crc.d \
./Core/Src/dac.d \
./Core/Src/fdcan.d \
./Core/Src/gpdma.d \
./Core/Src/gpio.d \
./Core/Src/icache.d \
./Core/Src/main.d \
./Core/Src/spi.d \
./Core/Src/stm32h5xx_hal_msp.d \
./Core/Src/stm32h5xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32h5xx.d \
./Core/Src/tim.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H563xx -c -I../Core/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I../Drivers/CMSIS/Include -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/can" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/fw-utils/bootloader" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/fw-utils/bootloader/fw_verification" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/fw-utils/btea" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/fw-utils/data_comm" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/fw-utils/nand_flash" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/fw-utils/packet2" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/fw-utils/tinycrypt/lib/include" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/services/memory" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/sf_hal_stm32h5/bootloader_hal" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/sf_hal_stm32h5/can_hal" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/sf_hal_stm32h5/crc_hal" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/sf_hal_stm32h5/flash_hal" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/sf_hal_stm32h5/gpio_hal" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/sf_hal_stm32h5/timer_hal" -I"F:/home/marc-ros/STM32CubeIDE/workspace_1.18.1_new/varg-stm32h5-bootloader/sf_hal_charger_board/led_hal" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/adc.cyclo ./Core/Src/adc.d ./Core/Src/adc.o ./Core/Src/adc.su ./Core/Src/crc.cyclo ./Core/Src/crc.d ./Core/Src/crc.o ./Core/Src/crc.su ./Core/Src/dac.cyclo ./Core/Src/dac.d ./Core/Src/dac.o ./Core/Src/dac.su ./Core/Src/fdcan.cyclo ./Core/Src/fdcan.d ./Core/Src/fdcan.o ./Core/Src/fdcan.su ./Core/Src/gpdma.cyclo ./Core/Src/gpdma.d ./Core/Src/gpdma.o ./Core/Src/gpdma.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/icache.cyclo ./Core/Src/icache.d ./Core/Src/icache.o ./Core/Src/icache.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/spi.cyclo ./Core/Src/spi.d ./Core/Src/spi.o ./Core/Src/spi.su ./Core/Src/stm32h5xx_hal_msp.cyclo ./Core/Src/stm32h5xx_hal_msp.d ./Core/Src/stm32h5xx_hal_msp.o ./Core/Src/stm32h5xx_hal_msp.su ./Core/Src/stm32h5xx_it.cyclo ./Core/Src/stm32h5xx_it.d ./Core/Src/stm32h5xx_it.o ./Core/Src/stm32h5xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32h5xx.cyclo ./Core/Src/system_stm32h5xx.d ./Core/Src/system_stm32h5xx.o ./Core/Src/system_stm32h5xx.su ./Core/Src/tim.cyclo ./Core/Src/tim.d ./Core/Src/tim.o ./Core/Src/tim.su

.PHONY: clean-Core-2f-Src

