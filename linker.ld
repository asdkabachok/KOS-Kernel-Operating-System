/*
 * KOS Kernel Linker Script
 * Определяет структуру памяти ядра для x86_64 higher-half layout
 * 
 * Версия: 1.0
 * Для Acer Nitro 5 (Ryzen 5 3550H)
 */

/* Точка входа — должна совпадать с boot.asm */
ENTRY(_start)

/* Базовый адрес для виртуальной памяти ядра (higher half) */
KERNEL_VIRT_BASE = 0xFFFF800000000000;

/* Физический адрес, куда загружается ядро (1MB) */
KERNEL_LOAD_BASE = 0x100000;

SECTIONS
{
    /* Виртуальный адрес начала ядра */
    . = KERNEL_VIRT_BASE;

    /* Начало ядра — для отладки и статистики */
    _kernel_start = .;

    /* Секция текста (код) — выравнивание 4KB */
    .text ALIGN(4K) : AT(LOADADDR(.text)) {
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    }

    /* Секция данных (инициализированные переменные) */
    .data ALIGN(4K) : AT(LOADADDR(.data)) {
        *(.data)
        *(.data.*)
    }

    /* Секция BSS (неинициализированные данные) */
    .bss ALIGN(4K) : AT(LOADADDR(.bss)) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /* Секция для данных только для чтения после инициализации */
    .rodata2 ALIGN(4K) : AT(LOADADDR(.rodata2)) {
        *(.rodata2)
    }

    /* Конец загруженной части ядра */
    _kernel_end = .;
    _kernel_size = _kernel_end - _kernel_start;

    /* Дебаг информация (не загружается в память) */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note.GNU-stack)
        *(.comment)
    }

    /* Debug sections — если нужны */
    .debug_info  0 : { *(.debug_info) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_line 0 : { *(.debug_line) }
    .debug_str 0 : { *(.debug_str) }
    .debug_frame 0 : { *(.debug_frame) }
    .debug_loc 0 : { *(.debug_loc) }
    .debug_macinfo 0 : { *(.debug_macinfo) }
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames 0 : { *(.debug_varnames) }
}
