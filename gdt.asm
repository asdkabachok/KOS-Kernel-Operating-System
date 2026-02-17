; gdt.asm — Global Descriptor Table (GDT) для x86_64
; Полная реализация с TSS и user mode сегментами

section .rodata

; Макрос для создания GDT дескриптора
%macro GDT_ENTRY 2
    dw %1 & 0xFFFF
    dw 0x0000
    db 0x00
    db %2
    db 0x00
    db 0x00
%endmacro

; Глобальные метки
global gdt64_start
global gdt64_descriptor
global tss_start
global tss_ptr

; Таблица GDT
gdt64_start:
    ; Дескриптор 0x00 — Null descriptor (обязателен)
    dq 0

    ; Дескриптор 0x08 — Kernel Code Segment (64-bit)
    ; Base: 0x00000000, Limit: 0xFFFFF (4GB), Type: Code, DPL: 00
    dw 0xFFFF       ; Limit [15:0]
    dw 0x0000       ; Base [15:0]
    db 0x00         ; Base [23:16]
    db 10011010b    ; P=1, DPL=00, S=1, Type=1010 (Code, Readable)
    db 11001111b    ; G=1, D=0, L=1, AVL=0, Limit [19:16]
    db 0x00         ; Base [31:24]

    ; Дескриптор 0x10 — Kernel Data Segment
    ; Base: 0x00000000, Limit: 0xFFFFF (4GB), Type: Data, DPL: 00
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b    ; P=1, DPL=00, S=1, Type=0010 (Data, Writable)
    db 11001111b
    db 0x00

    ; Дескриптор 0x18 — User Code Segment (64-bit)
    ; Type: Code, DPL: 11 (User mode)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 11111010b    ; P=1, DPL=11, S=1, Type=1010 (Code, Readable)
    db 11001111b
    db 0x00

    ; Дескриптор 0x20 — User Data Segment
    ; Type: Data, DPL: 11 (User mode)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 11110010b    ; P=1, DPL=11, S=1, Type=0010 (Data, Writable)
    db 11001111b
    db 0x00

    ; Дескриптор 0x28 — TSS (Task State Segment)
    ; 64-bit TSS с IST поддержкой
    ; Размер: 104 байта (минимальный для 64-bit TSS)
    dw (tss_end - tss_start - 1) & 0xFFFF
    dw tss_start & 0xFFFF
    db (tss_start >> 16) & 0xFF
    db 10001001b    ; P=1, DPL=00, S=0, Type=1001 (32-bit TSS)
    db 0x00         ; Reserved
    db (tss_start >> 24) & 0xFF
    dd (tss_start >> 32) & 0xFFFFFFFF
    dd 0            ; Reserved

gdt64_end:

; Указатель на GDT
gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start

section .bss
align 16

; TSS (Task State Segment) — 104 байта
tss_start:
    ; RSP0 — указатель стека для кольца 0 (kernel stack)
    resq 1
    ; RSP1, RSP2 — для колец 1 и 2 (не используем)
    resq 1
    resq 1
    ; Reserved
    resq 1
    ; IST1-7 — Interrupt Stack Tables
    resq 1
    resq 1
    resq 1
    resq 1
    resq 1
    resq 1
    resq 1
    ; Reserved
    resq 1
    ; I/O Map Base Address (0xFFFF = no I/O permission map)
    resw 1
    resw 1         ; Reserved
tss_end:

; Указатель на TSS
tss_ptr:
    dw tss_end - tss_start - 1
    dq tss_start

section .text
bits 64

; gdt64_init — инициализация GDT и перезагрузка сегментов
global gdt64_init
gdt64_init:
    ; Загружаем GDT
    lgdt [gdt64_descriptor]
    
    ; Перезагружаем сегменты данных
    ; Загружаем DS, ES, FS, GS, SS нулевым селектором
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump для перезагрузки CS
    push 0x08                ; Селектор кода ядра
    lea rax, [rel .reload_cs]
    push rax
    retfq
    
.reload_cs:
    ; CS теперь 0x08 (kernel code)
    ret

; init_tss — инициализация TSS
; Аргумент: rdi = RSP0 (указатель стека ядра)
global init_tss
init_tss:
    ; Сохраняем RSP0 в TSS
    mov [tss_start], rdi
    
    ; Загружаем Task Register (TR)
    mov ax, 0x28            ; Селектор TSS в GDT
    ltr ax
    
    ret

; tss_set_rsp0 — установка RSP0 для текущего CPU
; Аргумент: rdi = новый RSP0
global tss_set_rsp0
tss_set_rsp0:
    mov [tss_start], rdi
    ret

; tss_get_rsp0 — получение текущего RSP0
; Возвращает: rax = текущий RSP0
global tss_get_rsp0
tss_get_rsp0:
    mov rax, [tss_start]
    ret

; Установка IST (Interrupt Stack Table) для конкретного прерывания
; Аргументы: rdi = номер IST (1-7), rsi = указатель стека
global tss_set_ist
tss_set_ist:
    ; IST1 находится по смещению 0x20 (32 байта) от начала TSS
    ; Каждый IST = 8 байт
    lea rax, [tss_start + 32]
    ; rdi = номер IST (1-7), умножаем на 8
    dec rdi                 ; 1-7 -> 0-6
    shl rdi, 3              ; умножаем на 8
    add rax, rdi            ; получаем адрес в TSS
    mov [rax], rsi          ; записываем указатель стека
    ret
