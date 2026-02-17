; io.asm - Операции ввода-вывода через порты
; Функции для работы с портами ввода-вывода (x86)

section .text

; Чтение байта из порта
; uint8_t inb(uint16_t port)
global inb
inb:
    mov dx, [esp+4]    ; Номер порта
    xor eax, eax
    in al, dx
    ret

; Запись байта в порт
; void outb(uint16_t port, uint8_t value)
global outb
outb:
    mov dx, [esp+4]    ; Номер порта
    mov al, [esp+8]    ; Значение
    out dx, al
    ret

; Чтение слова (16 бит) из порта
; uint16_t inw(uint16_t port)
global inw
inw:
    mov dx, [esp+4]    ; Номер порта
    xor eax, eax
    in ax, dx
    ret

; Запись слова (16 бит) в порт
; void outw(uint16_t port, uint16_t value)
global outw
outw:
    mov dx, [esp+4]    ; Номер порта
    mov ax, [esp+8]    ; Значение
    out dx, ax
    ret

; Чтение двойного слова (32 бита) из порта
; uint32_t inl(uint16_t port)
global inl
inl:
    mov dx, [esp+4]    ; Номер порта
    xor eax, eax
    in eax, dx
    ret

; Запись двойного слова (32 бита) в порт
; void outl(uint16_t port, uint32_t value)
global outl
outl:
    mov dx, [esp+4]    ; Номер порта
    mov eax, [esp+8]   ; Значение
    out dx, eax
    ret

; Чтение байта из порта с задержкой
; Используется для старых контроллеров, требующих задержки
global inb_p
inb_p:
    mov dx, [esp+4]
    xor eax, eax
    in al, dx
    ; Задержка - чтение из порта 0x80
    in al, 0x80
    ret

; Запись байта в порт с задержкой
global outb_p
outb_p:
    mov dx, [esp+4]
    mov al, [esp+8]
    out dx, al
    ; Задержка
    in al, 0x80
    ret

; Чтение из MMIO регистра (Memory-Mapped I/O)
; uint32_t mmio_read32(volatile uint32_t* addr)
global mmio_read32
mmio_read32:
    mov eax, [esp+4]
    mov eax, [eax]
    ret

; Запись в MMIO регистр
; void mmio_write32(volatile uint32_t* addr, uint32_t value)
global mmio_write32
mmio_write32:
    mov eax, [esp+8]
    mov ecx, [esp+4]
    mov [ecx], eax
    ret

; Чтение 64-бит из MMIO
; uint64_t mmio_read64(volatile uint64_t* addr)
global mmio_read64
mmio_read64:
    mov eax, [esp+4]
    mov eax, [eax]
    ret

; Запись 64-бит в MMIO
; void mmio_write64(volatile uint64_t* addr, uint64_t value)
global mmio_write64
mmio_write64:
    ; Используем регистровую конвенцию SysV AMD64
    ; rdi = addr, rsi = value
    mov rax, rsi
    mov [rdi], rax
    ret

; Отключение прерываний
; void disable_interrupts(void)
global disable_interrupts
disable_interrupts:
    cli
    ret

; Включение прерываний
; void enable_interrupts(void)
global enable_interrupts
enable_interrupts:
    sti
    ret

; Получение состояния флага прерываний
; uint64_t get_interrupt_state(void)
global get_interrupt_state
get_interrupt_flags:
    pushf
    pop eax
    and eax, 0x200      ; Проверяем флаг IF
    ret

; Остановка процессора (halt)
; void hlt(void)
global hlt
hlt:
    hlt
    ret

; Остановка процессора с разрешенными прерываниями
; void halt(void)
global halt
halt:
    sti
    hlt
    ret

; Waiting for interrupt
; void wait_for_interrupt(void)
global wait_for_interrupt
wait_for_interrupt:
    hlt
    ret

; Получение текущего значения CR0
; uint64_t get_cr0(void)
global get_cr0
get_cr0:
    mov eax, cr0
    ret

; Установка CR0
; void set_cr0(uint64_t value)
global set_cr0
set_cr0:
    mov eax, [esp+4]
    mov cr0, eax
    ret

; Получение текущего значения CR2 (адрес ошибки страницы)
; uint64_t get_cr2(void)
global get_cr2
get_cr2:
    mov eax, cr2
    ret

; Получение текущего значения CR4
; uint64_t get_cr4(void)
global get_cr4
get_cr4:
    mov eax, cr4
    ret

; Установка CR4
; void set_cr4(uint64_t value)
global set_cr4
set_cr4:
    mov eax, [esp+4]
    mov cr4, eax
    ret

; Получение текущего значения EFLAGS
; uint64_t get_eflags(void)
global get_eflags
get_eflags:
    pushf
    pop eax
    ret

; Установка EFLAGS
; void set_eflags(uint64_t value)
global set_eflags
set_eflags:
    mov eax, [esp+4]
    push eax
    popf
    ret
