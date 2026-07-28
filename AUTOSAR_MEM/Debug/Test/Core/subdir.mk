################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Test/Core/TestManager.c \
../Test/Core/main.c 

OBJS += \
./Test/Core/TestManager.o \
./Test/Core/main.o 

C_DEPS += \
./Test/Core/TestManager.d \
./Test/Core/main.d 


# Each subdirectory must supply rules for building sources it contributes
Test/Core/%.o Test/Core/%.su Test/Core/%.cyclo: ../Test/Core/%.c Test/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Testcase/include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Core" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Core/Inc" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/CMSIS/Device/ST/STM32F4xx/Include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Drivers/CMSIS/Include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Stub/Det" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/Code_Generator/templates/include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Test-2f-Core

clean-Test-2f-Core:
	-$(RM) ./Test/Core/TestManager.cyclo ./Test/Core/TestManager.d ./Test/Core/TestManager.o ./Test/Core/TestManager.su ./Test/Core/main.cyclo ./Test/Core/main.d ./Test/Core/main.o ./Test/Core/main.su

.PHONY: clean-Test-2f-Core

