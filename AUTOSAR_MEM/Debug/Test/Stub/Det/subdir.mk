################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Test/Stub/Det/det.c 

OBJS += \
./Test/Stub/Det/det.o 

C_DEPS += \
./Test/Stub/Det/det.d 


# Each subdirectory must supply rules for building sources it contributes
Test/Stub/Det/%.o Test/Stub/Det/%.su Test/Stub/Det/%.cyclo: ../Test/Stub/Det/%.c Test/Stub/Det/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -c -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Testcase/include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Core" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/STM32F401/Core/Inc" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Test/Stub/Det" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/include" -I"C:/Users/ZEPHYRUS M16/Documents/Intergration/AUTOSAR_MEM/Driver/Code_Generator/templates/include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Test-2f-Stub-2f-Det

clean-Test-2f-Stub-2f-Det:
	-$(RM) ./Test/Stub/Det/det.cyclo ./Test/Stub/Det/det.d ./Test/Stub/Det/det.o ./Test/Stub/Det/det.su

.PHONY: clean-Test-2f-Stub-2f-Det

