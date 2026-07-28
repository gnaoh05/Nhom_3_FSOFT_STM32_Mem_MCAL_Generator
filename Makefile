SHELL = cmd.exe

# 1. Tên Output File
TARGET = Nhom_3_FSOFT_STM32_Mem_MCAL

# 2. Thư mục chứa kết quả build
BUILD_DIR = build

# 3. Trình biên dịch & Toolchain (ARM GCC)
CC     = arm-none-eabi-gcc
AS     = arm-none-eabi-gcc -x assembler-with-cpp
CP     = arm-none-eabi-objcopy
SZ     = arm-none-eabi-size
HEX    = $(CP) -O ihex
BIN    = $(CP) -O binary -S

CPU       = -mcpu=cortex-m4
FPU       = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU       = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

C_SOURCES = \
Driver/src/Flash_IP.c \
Driver/src/Mem_IPW.c \
Test/Core/main.c \
Test/Stub/Det/det.c

C_INCLUDES = \
-IDriver/include \
-ITest/Generated_File \
-ITest/Stub/Det \
-ITest/Core

CFLAGS  = $(MCU) $(C_INCLUDES) -O0 -g3 -Wall -fdata-sections -ffunction-sections
# Đã thêm --specs=nosys.specs để sửa lỗi undefined reference to _exit
LDFLAGS = $(MCU) -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs

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