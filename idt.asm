; idt.asm — Interrupt Descriptor Table (IDT) для x86_64
; Полная реализация обработчиков прерываний и исключений

section .text
bits 64

; Экспортируем функции
global idt_install
global idt_set_gate
global interrupt_handler

; Импорты из C
extern idt_ptr
extern isr_handler
extern irq_handler

; IDT установка — загрузка таблицы прерываний
idt_install:
    ; Загружаем IDT
    lidt [idt_ptr]
    
    ; Включаем прерывания
    sti
    
    ret

; idt_set_gate — установка шлюза прерывания
; Аргументы: rdi = адрес обработчика, rsi = номер вектора, rdx = атрибуты
idt_set_gate:
    ; rdi = адрес обработчика
    ; rsi = номер вектора
    ; rdx = type_attr
    
    ; Получаем указатель на запись IDT
    lea rax, [idt_entries]
    mov rax, [rax]              ; Загружаем базу IDT
    mov ecx, esi                ; Номер вектора
    shl rcx, 4                 ; Умножаем на 16 (размер записи)
    add rax, rcx               ; Адрес записи
    
    ; Заполняем запись IDT
    ; Offset [15:0]
    mov [rax], di
    
    ; Offset [31:16]
    mov [rax + 0x02], dx
    shr rdx, 16
    mov [rax + 0x06], dx
    
    ; Selector (кодовый сегмент ядра)
    mov word [rax + 0x08], 0x08
    
    ; IST (0 = no IST)
    xor rdx, rdx
    mov byte [rax + 0x09], dl
    
    ; Type Attr
    mov byte [rax + 0x0B], 0x8E    ; P=1, DPL=00, Type=1110 (Interrupt Gate)
    
    ; Offset [63:32]
    shr rdi, 32
    mov [rax + 0x0C], edi
    
    ; Reserved
    mov dword [rax + 0x10], 0
    
    ret

; ============================================================
; Обработчики исключений CPU (0-31)
; ============================================================

; Макрос для создания ISR без кода ошибки
%macro ISR_NOERR 1
global isr%1
isr%1:
    push 0                   ; Код ошибки (нет)
    push %1                  ; Номер вектора
    jmp isr_common
%endmacro

; Макрос для создания ISR с кодом ошибки
%macro ISR_ERR 1
global isr%1
isr%1:
    push %1                  ; Номер вектора
    jmp isr_common
%endmacro

; Все исключения CPU
ISR_NOERR 0     ; #DE — Divide Error
ISR_NOERR 1     ; #DB — Debug Exception
ISR_NOERR 2     ; NMI Interrupt
ISR_NOERR 3     ; #BP — Breakpoint
ISR_NOERR 4     ; #OF — Overflow
ISR_NOERR 5     ; #BR — BOUND Range Exceeded
ISR_NOERR 6     ; #UD — Invalid Opcode
ISR_NOERR 7     ; #NM — Device Not Available
ISR_ERR  8     ; #DF — Double Fault (has error code)
ISR_NOERR 9     ; Coprocessor Segment Overrun (obsolete)
ISR_ERR  10    ; #TS — Invalid TSS
ISR_ERR  11    ; #NP — Segment Not Present
ISR_ERR  12    ; #SS — Stack Segment Fault
ISR_ERR  13    ; #GP — General Protection Fault
ISR_ERR  14    ; #PF — Page Fault
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; #MF — x87 FPU Floating-Point Error
ISR_ERR  17    ; #AC — Alignment Check
ISR_NOERR 18    ; #MC — Machine Check
ISR_NOERR 19    ; #XF — SIMD Floating-Point Exception
ISR_NOERR 20    ; Virtualization Exception
ISR_NOERR 21    ; Reserved
ISR_NOERR 22    ; Reserved
ISR_NOERR 23    ; Reserved
ISR_NOERR 24    ; Reserved
ISR_NOERR 25    ; Reserved
ISR_NOERR 26    ; Reserved
ISR_NOERR 27    ; Reserved
ISR_NOERR 28    ; Reserved
ISR_ERR  29    ; #VE — Virtualization Exception
ISR_NOERR 30    ; Reserved
ISR_NOERR 31    ; Reserved

; Общий обработчик для исключений
isr_common:
    ; Сохраняем регистры на стеке
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; RSP указывает на структуру сохранённых регистров
    ; +120 = номер вектора
    ; +128 = код ошибки
    
    ; Вызываем C обработчик
    mov rdi, rsp              ; Указатель на struct registers
    call isr_handler
    
    ; Восстанавливаем регистры и возвращаемся
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    
    ; Добавляем 16 байт (номер вектора + код ошибки)
    add rsp, 16
    
    ; Возврат из прерывания
    iretq

; ============================================================
; Обработчики аппаратных прерываний IRQ (0-15)
; ============================================================

; Макрос для создания IRQ обработчика
%macro IRQ 1
global irq%1
irq%1:
    push 0                   ; Код ошибки
    push (32 + %1)           ; Номер вектора (IRQ + 32)
    jmp irq_common
%endmacro

; Все IRQ обработчики
IRQ 0    ; IRQ0 — системный таймер
IRQ 1    ; IRQ1 — клавиатура
IRQ 2    ; IRQ2 — каскад IRQ (IRQ 8-15)
IRQ 3    ; IRQ3 — COM2
IRQ 4    ; IRQ4 — COM1
IRQ 5    ; IRQ5 — LPT2
IRQ 6    ; IRQ6 — floppy disk
IRQ 7    ; IRQ7 — LPT1
IRQ 8    ; IRQ8 — RTC
IRQ 9    ; IRQ9 — ACPI
IRQ 10   ; IRQ10 — reserved
IRQ 11   ; IRQ11 — reserved
IRQ 12   ; IRQ12 — PS/2 mouse
IRQ 13   ; IRQ13 — FPU
IRQ 14   ; IRQ14 — primary ATA
IRQ 15   ; IRQ15 — secondary ATA

; Общий обработчик для IRQ
irq_common:
    ; Сохраняем все регистры
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; Вызываем C обработчик
    mov rdi, rsp
    call irq_handler
    
    ; Отправляем End of Interrupt (EOI)
    ; Проверяем какой контроллер прерываний вызвал
    ; Получаем номер IRQ из стека
    mov rax, [rsp + 120]     ; Номер вектора
    sub rax, 32              ; Получаем номер IRQ
    
    ; Если IRQ >= 8, нужно послать EOI на slave контроллер
    cmp al, 8
    jb .send_eoi_only
    
    ; EOI на slave PIC
    mov al, 0xA0
    out 0xA0, al
    
.send_eoi_only:
    ; EOI на master PIC
    mov al, 0x20
    out 0x20, al
    
    ; Восстанавливаем регистры
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    
    add rsp, 16
    iretq

; ============================================================
; Системный вызов (SYSCALL) — быстрый вход в kernel mode
; ============================================================

; обработчик syscall — номер 0x80
global isr_syscall
isr_syscall:
    ; Сохраняем регистры
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    
    ; RAX = номер syscall
    ; RDI, RSI, RDX, R10, R9, R8 = аргументы
    ; RFLAGS сохранён на стеке процессором
    
    ; Вызываем обработчик syscall
    ; extern void syscall_handler(struct registers* regs);
    mov rdi, rsp
    call syscall_handler
    
    ; Восстанавливаем регистры (кроме RAX — там результат)
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    
    ; RAX уже содержит результат от syscall_handler
    
    ; Возвращаемся через sysretq
    pop rbp
    
    ; Восстанавливаем RFLAGS
    ; (процессор не восстанавливает RFLAGS автоматически)
    
    ; Возврат в userspace
    mov rsp, rdi              ; Восстанавливаем RSP userspace
    
    add rsp, 8                ; Пропускаем сохранённый RFLAGS
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    
    ; sysret в userspace с RFLAGS
    ; Это упрощённая версия — полная нуждается в проверках
    iretq

section .data

; Таблица IDT (заполняется программно или здесь)
align 16
idt_entries_base:
    dq 0
    times 255 dq 0

idt_ptr:
    dw 256 * 16 - 1
    dq idt_entries_base
