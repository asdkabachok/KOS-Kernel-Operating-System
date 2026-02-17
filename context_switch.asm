; context_switch.asm — Переключение контекста процессов/потоков для x86_64
; Полноценная реализация для ядра

section .text
bits 64

; Экспортируем функции
global context_switch
global switch_to_user
global restore_kernel_context
global get_rip
global get_rsp
global get_cr3
global set_cr3
global invlpg
global rdmsr
global wrmsr

; Импорты
extern current_thread

; ============================================================
; context_switch — переключение между потоками
; Аргументы:
;   rdi = указатель на old thread (куда сохранить RSP)
;   rsi = указатель на new thread (откуда восстановить RSP)
;   rdx = CR3 нового потока (адрес таблицы страниц)
; ============================================================
context_switch:
    ; Сохраняем текущий контекст
    ; RSP текущего потока -> в структуру old thread
    
    ; Сохраняем регистры общего назначения
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    ; Сохраняем RSP в структуру old потока
    mov [rdi], rsp
    
    ; Если old == NULL (первый вызов), пропускаем
    test rdi, rdi
    jz .skip_save
    
.skip_save:
    ; Восстанавливаем RSP из структуры new потока
    mov rsp, rsi
    
    ; Если new_cr3 != 0, переключаем адресное пространство
    test rdx, rdx
    jz .skip_cr3
    
    ; Переключаем CR3
    mov cr3, rdx
    
.skip_cr3:
    ; Восстанавливаем регистры
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    
    ; Возвращаемся в новый поток
    ret

; ============================================================
; switch_to_user — переключение в пользовательский режим
; Аргументы:
;   rdi = пользовательский RSP
;   rsi = пользовательский RIP (точка входа)
;   rdx = адрес стека ядра для возврата (kernel rsp)
; ============================================================
switch_to_user:
    ; Сохраняем текущий RSP ядра для возврата
    mov [rdx], rsp
    
    ; Переключаемся на пользовательский стек
    mov rsp, rdi
    
    ; Настраиваем userspace сегменты
    mov ax, 0x23            ; User data selector (RPL=3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Подготавливаем стек для iret в userspace
    push 0x23               ; SS userspace
    push rdi                ; RSP userspace
    push rflags             ; RFLAGS
    push 0x1B               ; CS userspace (64-bit)
    push rsi                ; RIP userspace
    
    ; Возврат в userspace
    iretq

; ============================================================
; restore_kernel_context — возврат в режим ядра
; Аргументы:
;   rdi = указатель на структуру потока (с RSP)
; ============================================================
restore_kernel_context:
    ; Восстанавливаем RSP из структуры потока
    mov rsp, [rdi]
    
    ; Переключаемся на kernel сегменты
    mov ax, 0x10            ; Kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ret

; ============================================================
; get_rip — получение текущего RIP (для отладки)
; ============================================================
get_rip:
    lea rax, [rip]
rip:
    ret

; ============================================================
; get_rsp — получение текущего RSP
; ============================================================
get_rsp:
    mov rax, rsp
    ret

; ============================================================
; get_cr3 — получение текущего CR3
; ============================================================
get_cr3:
    mov rax, cr3
    ret

; ============================================================
; set_cr3 — установка нового CR3
; Аргументы:
;   rdi = новый CR3
; ============================================================
set_cr3:
    mov cr3, rdi
    ret

; ============================================================
; invlpg — инвалидация TLB для конкретного адреса
; Аргументы:
;   rdi = виртуальный адрес
; ============================================================
invlpg:
    invlpg [rdi]
    ret

; ============================================================
; rdmsr — чтение MSR
; Аргументы:
;   ecx = номер MSR
; Возвращает:
;   rax = значение MSR (rdx:rax)
; ============================================================
rdmsr:
    rdmsr
    ret

; ============================================================
; wrmsr — запись MSR
; Аргументы:
;   ecx = номер MSR
;   rax = значение (rdx:rax)
; ============================================================
wrmsr:
    wrmsr
    ret

; ============================================================
; read_cr0 / write_cr0 — управление CR0
; ============================================================
global read_cr0
read_cr0:
    mov rax, cr0
    ret

global write_cr0
write_cr0:
    mov cr0, rdi
    ret

; ============================================================
; read_cr2 / read_cr4
; ============================================================
global read_cr2
read_cr2:
    mov rax, cr2
    ret

global read_cr4
read_cr4:
    mov rax, cr4
    ret

global write_cr4
write_cr4:
    mov cr4, rdi
    ret

; ============================================================
; cpuid — получение информации о CPU
; ============================================================
global cpuid_func
cpuid_func:
    ; rax = function ID
    ; Результат: rax, rbx, rcx, rdx
    cpuid
    ret

section .data

; Глобальные переменные
global kernel_rsp0
kernel_rsp0:
    dq 0                    ; RSP0 для текущего CPU

global current_cpu
current_cpu:
    dq 0                    ; Указатель на текущую структуру cpu
