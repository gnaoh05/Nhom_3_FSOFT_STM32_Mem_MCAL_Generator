################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Driver/src/Flash_IP.c \
../Driver/src/Mem.c \
../Driver/src/Mem_IPW.c 

OBJS += \
./Driver/src/Flash_IP.o \
./Driver/src/Mem.o \
./Driver/src/Mem_IPW.o 

C_DEPS += \
./Driver/src/Flash_IP.d \
./Driver/src/Mem.d \
./Driver/src/Mem_IPW.d 


# Each subdirectory must supply rules for building sources it contributes
Driver/src/%.o Driver/src/%.su Driver/src/%.cyclo: ../Driver/src/%.c Driver/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Core/Inc" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/CMSIS/Device/ST/STM32F4xx/Include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/CMSIS/Include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/STM32F4xx_HAL_Driver/Inc" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Stub/Det" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/Code_Generator/templates/include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Driver-2f-src

clean-Driver-2f-src:
	-$(RM) ./Driver/src/Flash_IP.cyclo ./Driver/src/Flash_IP.d ./Driver/src/Flash_IP.o ./Driver/src/Flash_IP.su ./Driver/src/Mem.cyclo ./Driver/src/Mem.d ./Driver/src/Mem.o ./Driver/src/Mem.su ./Driver/src/Mem_IPW.cyclo ./Driver/src/Mem_IPW.d ./Driver/src/Mem_IPW.o ./Driver/src/Mem_IPW.su

.PHONY: clean-Driver-2f-src

