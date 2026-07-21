################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Test/Core/main.c 

OBJS += \
./Test/Core/main.o 

C_DEPS += \
./Test/Core/main.d 


# Each subdirectory must supply rules for building sources it contributes
Test/Core/%.o Test/Core/%.su Test/Core/%.cyclo: ../Test/Core/%.c Test/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Stub/Det" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/Code_Generator/templates/include" -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Test-2f-Core

clean-Test-2f-Core:
	-$(RM) ./Test/Core/main.cyclo ./Test/Core/main.d ./Test/Core/main.o ./Test/Core/main.su

.PHONY: clean-Test-2f-Core

