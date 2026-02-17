; boot.asm — Multiboot2 загрузчик для x86_64
; KOS Kernel Bootloader
; Версия: 1.0 для Acer Nitro 5 (Ryzen 5 3550H)

section .multiboot
align 8

; Multiboot2 заголовок
multiboot_header:
    ; Магическое число Multiboot2
    dd 0xE85250D6
    
    ; Архитектура: i386 (0)
    dd 0
    
    ; Длина заголовка
    dd multiboot_end - multiboot_header
    
    ; Контрольная сумма
    dd 0x100000000 - (0xE85250D6 + 0 + (multiboot_end - multiboot_header))

    ; Тег информации о загрузчике
    dw 1        ; Тип = информация о загрузчике
    dw 0        ; Флаги
    dd loader_tag_end - loader_tag
loader_tag:
    db "KOS 1.0", 0
loader_tag_end:

    ; Тег запроса информации о памяти
    dw 4        ; Тип = информация о памяти
    dw 0        ; Флаги
    dd mem_tag_end - mem_tag
mem_tag:
    dd 1        ; Min addr
    dd 0x100000 ; Max addr
mem_tag_end:

    ; Тег видеорежима
    dw 5        ; Тип = видеорежим
    dw 0        ; Флаги
    dd vbe_tag_end - vbe_tag
vbe_tag:
    dd 1024     ; Ширина
    dd 768      ; Высота
    dd 32       ; BPP
vbe_tag_end:

    ; Тег завершения
    dw 0        ; Тип = завершение
    dw 0        ; Флаги
    dd 8        ; Длина
multiboot_end:

; Секция BSS
section .bss
align 4096

; Page tables для early boot
global_boot_pml4:
    resq 512
global_boot_pdpt:
    resq 512
global_boot_pd:
    resq 512
global_boot_pt:
    resq 512

; Стек для загрузки
align 16
boot_stack_bottom:
    resb 16384
boot_stack_top:

section .rodata
; Константы
PRESENT         equ 0x001
WRITABLE        equ 0x002
HUGE_PAGE       equ 0x080

section .text
bits 32

global _start
extern kernel_main

_start:
    ; Сохраняем указатель на Multiboot2 информацию
    mov [multiboot_info_ptr], ebx
    mov [multiboot_magic], eax
    
    ; Устанавливаем стек
    mov esp, boot_stack_top
    cld
    
    ; Проверяем long mode
    call check_long_mode
    jc .no_long_mode
    
    ; Настраиваем paging
    call setup_paging
    
    ; Включаем PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Загружаем GDT
    lgdt [gdt32_ptr]
    
    ; Включаем long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Включаем paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ; Far jump в long mode
    jmp 0x08:.long_mode
    
.bits 64
.long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov rsp, boot_stack_top
    
    ; Переходим на ядро
    mov rdi, [multiboot_info_ptr]
    call kernel_main
    
    cli
.halt:
    hlt
    jmp .halt

.bits 32
.no_long_mode:
    mov esi, error_no_long_mode
    mov edi, 0xB8000
    mov ah, 0x04
.loop:
    lodsb
    test al, al
    jz .halt_error
    stosw
    jmp .loop
.halt_error:
    cli
    hlt
    jmp .halt_error

error_no_long_mode: db "ERROR: No Long Mode!", 0

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .fail
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .fail
    clc
    ret
.fail:
    stc
    ret

setup_paging:
    ; Очищаем таблицы
    mov edi, global_boot_pml4
    mov ecx, 512 * 4
    xor eax, eax
    rep stosd
    
    ; PML4[0] -> PDPT
    mov edi, global_boot_pml4
    mov eax, global_boot_pdpt
    or eax, PRESENT | WRITABLE
    mov [edi], eax
    
    ; PML4[256] -> PDPT (high half)
    mov edi, global_boot_pml4 + 256 * 8
    mov eax, global_boot_pdpt
    or eax, PRESENT | WRITABLE
    mov [edi], eax
    
    ; PDPT[0] -> PD
    mov edi, global_boot_pdpt
    mov eax, global_boot_pd
    or eax, PRESENT | WRITABLE
    mov [edi], eax
    
    ; PD[0] -> PT
    mov edi, global_boot_pd
    mov eax, global_boot_pt
    or eax, PRESENT | WRITABLE
    mov [edi], eax
    
    ; PD[1-511] -> 2MB huge pages
    mov edi, global_boot_pd + 8
    mov eax, 0x200000
    or eax, PRESENT | WRITABLE | HUGE_PAGE
    mov ecx, 511
.setup_huge:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .setup_huge
    
    ; PT[0-511] -> 4KB pages for first 2MB
    mov edi, global_boot_pt
    xor eax, eax
    or eax, PRESENT | WRITABLE
    mov ecx, 512
.setup_4k:
    mov [edi], eax
    add eax, 0x1000
    add edi, 8
    loop .setup_4k
    
    mov eax, global_boot_pml4
    mov cr3, eax
    ret

section .data
align 16
gdt32_start:
    dq 0
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt32_end:

gdt32_ptr:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start

multiboot_info_ptr:
    dq 0
multiboot_magic:
    dd 0
