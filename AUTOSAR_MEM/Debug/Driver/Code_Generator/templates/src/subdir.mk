################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Driver/Code_Generator/templates/src/Flash_IP_Cfg.c \
../Driver/Code_Generator/templates/src/Mem_Cfg.c 

OBJS += \
./Driver/Code_Generator/templates/src/Flash_IP_Cfg.o \
./Driver/Code_Generator/templates/src/Mem_Cfg.o 

C_DEPS += \
./Driver/Code_Generator/templates/src/Flash_IP_Cfg.d \
./Driver/Code_Generator/templates/src/Mem_Cfg.d 


# Each subdirectory must supply rules for building sources it contributes
Driver/Code_Generator/templates/src/%.o Driver/Code_Generator/templates/src/%.su Driver/Code_Generator/templates/src/%.cyclo: ../Driver/Code_Generator/templates/src/%.c Driver/Code_Generator/templates/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Core/Inc" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/CMSIS/Device/ST/STM32F4xx/Include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/CMSIS/Include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/STM32F4xx_HAL_Driver/Inc" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Stub/Det" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/Code_Generator/templates/include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Driver-2f-Code_Generator-2f-templates-2f-src

clean-Driver-2f-Code_Generator-2f-templates-2f-src:
	-$(RM) ./Driver/Code_Generator/templates/src/Flash_IP_Cfg.cyclo ./Driver/Code_Generator/templates/src/Flash_IP_Cfg.d ./Driver/Code_Generator/templates/src/Flash_IP_Cfg.o ./Driver/Code_Generator/templates/src/Flash_IP_Cfg.su ./Driver/Code_Generator/templates/src/Mem_Cfg.cyclo ./Driver/Code_Generator/templates/src/Mem_Cfg.d ./Driver/Code_Generator/templates/src/Mem_Cfg.o ./Driver/Code_Generator/templates/src/Mem_Cfg.su

.PHONY: clean-Driver-2f-Code_Generator-2f-templates-2f-src

