SHELL = cmd.exe

# 1. Tên Output File
TARGET = Nhom_3_FSOFT_STM32_Mem_MCAL

# 2. Thư mục chứa kết quả build
BUILD_DIR = build

# 3. Trình biên dịch & Toolchain (ARM GCC từ STM32CubeIDE)
TOOLCHAIN_PATH = C:/ST/STM32CubeIDE_1.14.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.11.3.rel1.win32_1.1.100.202309141235/tools/bin/
CC     = $(TOOLCHAIN_PATH)arm-none-eabi-gcc
AS     = $(TOOLCHAIN_PATH)arm-none-eabi-gcc -x assembler-with-cpp
CP     = $(TOOLCHAIN_PATH)arm-none-eabi-objcopy
SZ     = $(TOOLCHAIN_PATH)arm-none-eabi-size
HEX    = $(CP) -O ihex
BIN    = $(CP) -O binary -S

# 4. Cấu hình Phần cứng (STM32F401)
CPU       = -mcpu=cortex-m4
FPU       = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU       = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# 5. Danh sách file C cần biên dịch
C_SOURCES = \
Driver/src/Flash_IP.c \
Driver/src/Mem_IPW.c \
Test/Core/main.c \
Test/Stub/Det/det.c

# 6. Danh sách Include Directories (Đã lấy trọn bộ toàn bộ thư mục Stub)
C_INCLUDES = \
-IDriver/include \
-ITest/Generated_File \
-ITest/Core \
-ITest/Stub \
-ITest/Stub/std \
-ITest/Stub/Det\
-ITest/Stub/Det/MemAcc\
-ITest/Stub/Det/SchM

# 7. Cấu hình Linker Script
LDSCRIPT = Test/Core/STM32F401RETx_FLASH.ld

# 8. Flag Biên dịch & Linker
CFLAGS  = $(MCU) $(C_INCLUDES) -O0 -g3 -Wall -fdata-sections -ffunction-sections
LDFLAGS = $(MCU) -T$(LDSCRIPT) -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs

# 9. Gom file Object
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo [CC] $<
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo [LINK] $@
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo [SIZE]
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(BIN) $< $@

# Tạo thư mục build
$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

# Lệnh clean
clean:
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"

.PHONY: all clean