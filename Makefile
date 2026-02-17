# ==========================================
#   KOS Kernel Build System
#   Сборочная система для ядра KOS
# ==========================================

# --- Определение инструментов ---
# Кросс-компилятор для x86_64
CC      := x86_64-elf-gcc
# Ассемблер
AS      := nasm
# Линковщик
LD      := x86_64-elf-ld
# Утилита для создания ISO
GRUB    := grub-mkrescue
# Эмулятор
QEMU    := qemu-system-x86_64

# --- Структура проекта ---
# Корневая директория проекта
ROOT_DIR    := .
# Директория для сборки
BUILD_DIR   := build
# Директория для ISO образа
ISO_DIR     := isodir
# Итоговый бинарник ядра
TARGET      := kernel.bin
# Имя ISO образа
ISO_NAME    := kos.iso
# Скрипт линковки
LINKER      := linker.ld

# --- Обнаружение исходных файлов ---
# Все .c файлы в корневой директории
C_SRCS      := $(wildcard $(ROOT_DIR)/*.c)
# Все .asm файлы в корневой директории
ASM_SRCS    := $(wildcard $(ROOT_DIR)/*.asm)

# --- Генерация объектных файлов ---
# Преобразование .c -> .o с сохранением в build/
C_OBJS      := $(patsubst $(ROOT_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SRCS))
# Преобразование .asm -> .o с сохранением в build/
ASM_OBJS    := $(patsubst $(ROOT_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS))
# Все объектные файлы
ALL_OBJS    := $(C_OBJS) $(ASM_OBJS)

# --- Файлы зависимостей ---
DEPS        := $(C_OBJS:.o=.d)

# --- Флаги компиляции ---
# CFLAGS: -ffreestanding (независимая среда), -mno-red-zone (без red zone),
#         -mcmodel=kernel (модель ядра), -std=gnu11 (стандарт C11),
#         -O2 (оптимизация), -Wall -Wextra (предупреждения),
#         -m64 -march=x86_64 (архитектура)
CFLAGS      := -ffreestanding -mno-red-zone -mcmodel=kernel \
               -std=gnu11 -O2 -Wall -Wextra -m64 -march=x86-64 \
               -MMD -MP -I$(ROOT_DIR)

# --- Флаги ассемблирования ---
# ASFLAGS: -f elf64 (64-битный формат ELF), -g (отладочная информация)
ASFLAGS     := -f elf64 -g

# --- Флаги линковки ---
# LDFLAGS: -T linker.ld (скрипт линковки), -nostdlib (без стандартной библиотеки)
LDFLAGS     := -T $(LINKER) -nostdlib -z max-page-size=0x1000

# --- Флаги QEMU ---
# Запуск с CDROM, 4GB RAM, CPU host, KVM, 4 ядра, serial console
QEMU_FLAGS  := -cdrom $(ISO_NAME) \
               -m 4G \
               -cpu host \
               -enable-kvm \
               -smp 4 \
               -serial stdio

# ==========================================
#   Цели сборки (Targets)
# ==========================================

# По умолчанию собираем ядро
all: $(TARGET)

# Линковка всех объектных файлов в kernel.bin
$(TARGET): $(ALL_OBJS) $(LINKER)
	@echo "[LD] Линковка ядра $(TARGET)..."
	@$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)
	@echo "[OK] Ядро успешно собрано!"

# --- Компиляция C файлов ---
$(BUILD_DIR)/%.o: $(ROOT_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC] $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

# --- Ассемблирование ASM файлов ---
$(BUILD_DIR)/%.o: $(ROOT_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "[AS] $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

# --- Создание ISO образа ---
iso: $(ISO_NAME)

$(ISO_NAME): $(TARGET)
	@echo "[ISO] Создание образа $(ISO_NAME)..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(TARGET) $(ISO_DIR)/boot/
	@# Используем существующий grub.cfg или создаем базовый
	@if [ -f $(ROOT_DIR)/grub.cfg ]; then \
		cp $(ROOT_DIR)/grub.cfg $(ISO_DIR)/boot/grub/; \
		echo "[ISO] Использован grub.cfg"; \
	else \
		echo "[ISO] Создание базового grub.cfg..."; \
		echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg; \
		echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg; \
		echo 'menuentry "KOS Kernel" {' >> $(ISO_DIR)/boot/grub/grub.cfg; \
		echo '  multiboot2 /boot/kernel.bin' >> $(ISO_DIR)/boot/grub/grub.cfg; \
		echo '  boot' >> $(ISO_DIR)/boot/grub/grub.cfg; \
		echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg; \
	fi
	@$(GRUB) -o $(ISO_NAME) $(ISO_DIR)
	@echo "[OK] ISO образ создан: $(ISO_NAME)"

# --- Запуск в QEMU ---
run: iso
	@echo "[QEMU] Запуск ядра в эмуляторе..."
	@$(QEMU) $(QEMU_FLAGS)

# --- Установка ядра в систему ---
install: $(TARGET)
	@echo "[INSTALL] Установка ядра в /boot (требуются права root)..."
	@sudo cp $(TARGET) /boot/kos-kernel.bin
	@echo "[OK] Ядро установлено как /boot/kos-kernel.bin"

# --- Очистка артефактов сборки ---
clean:
	@echo "[CLEAN] Удаление артефактов сборки..."
	@rm -rf $(BUILD_DIR) $(TARGET) $(ISO_NAME) $(ISO_DIR)
	@echo "[OK] Очистка завершена"

# --- Пересборка ---
rebuild: clean all
	@echo "[OK] Пересборка завершена"

# --- Информация о проекте ---
info:
	@echo "=========================================="
	@echo "   KOS Build System - Информация"
	@echo "=========================================="
	@echo "Корневая директория: $(ROOT_DIR)"
	@echo "Директория сборки:   $(BUILD_DIR)"
	@echo ""
	@echo "Исходные файлы C:"
	@$(foreach src,$(C_SRCS),echo "  $(src)";)
	@echo ""
	@echo "Исходные файлы ASM:"
	@$(foreach src,$(ASM_SRCS),echo "  $(src)";)
	@echo ""
	@echo "Объектные файлы:"
	@$(foreach obj,$(ALL_OBJS),echo "  $(obj)";)
	@echo ""
	@echo "Флаги компилятора:"
	@echo "$(CFLAGS)"
	@echo ""
	@echo "Флаги QEMU:"
	@echo "$(QEMU_FLAGS)"
	@echo "=========================================="

# Включаем файлы зависимостей
-include $(DEPS)

# ==========================================
#   Конец Makefile
# ==========================================
