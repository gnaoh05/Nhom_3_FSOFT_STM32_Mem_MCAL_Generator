################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Stub/Det/det.c 

OBJS += \
./Stub/Det/det.o 

C_DEPS += \
./Stub/Det/det.d 


# Each subdirectory must supply rules for building sources it contributes
Stub/Det/%.o Stub/Det/%.su Stub/Det/%.cyclo: ../Stub/Det/%.c Stub/Det/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/Code_Generator/templates/include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Stub/Det" -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/include" -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Stub-2f-Det

clean-Stub-2f-Det:
	-$(RM) ./Stub/Det/det.cyclo ./Stub/Det/det.d ./Stub/Det/det.o ./Stub/Det/det.su

.PHONY: clean-Stub-2f-Det

