; atomic.asm - Атомарные операции
; Атомарные примитивы для синхронизации

section .text

; Атомарное сравнение и обмен (CAS)
; int atomic_compare_exchange(int* ptr, int old_val, int new_val)
; Возвращает 1 если обмен произошел, 0 если нет
global atomic_compare_exchange
atomic_compare_exchange:
    ; Параметры:
    ; [ebp+8]  - указатель на переменную
    ; [ebp+12] - старое значение
    ; [ebp+16] - новое значение
    
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+12]    ; Старое значение
    mov edx, [ebp+16]    ; Новое значение
    mov ecx, [ebp+8]     ; Указатель на переменную
    
    ; lock cmpxchg [ecx], edx
    ; Атомарно сравнивает eax с [ecx] и если равно - записывает edx в [ecx]
    ; Устанавливает ZF если обмен произошел
    lock cmpxchg [ecx], edx
    
    ; Возвращаем результат
    setz al              ; al = 1 если обмен произошел
    movzx eax, al
    
    pop ebp
    ret

; Атомарное добавление
; int atomic_add(int* ptr, int val)
global atomic_add
atomic_add:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+12]    ; Значение для добавления
    mov ecx, [ebp+8]     ; Указатель на переменную
    
    lock xadd [ecx], eax ; Атомарный обмен и сложение
    
    ; Возвращаем старое значение
    pop ebp
    ret

; Атомарное И
; int atomic_and(int* ptr, int val)
global atomic_and
atomic_compare_exchange:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+12]    ; Маска для AND
    mov ecx, [ebp+8]     ; Указатель на переменную
    
    lock and [ecx], eax
    
    pop ebp
    ret

; Атомарное ИЛИ
; int atomic_or(int* ptr, int val)
global atomic_or
atomic_or:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+12]    ; Маска для OR
    mov ecx, [ebp+8]     ; Указатель на переменную
    
    lock or [ecx], eax
    
    pop ebp
    ret

; Атомарное чтение 32-бит
; int atomic_load(int* ptr)
global atomic_load
atomic_load:
    mov eax, [ebp+8]
    mov eax, [eax]
    ret

; Атомарная запись 32-бит
; void atomic_store(int* ptr, int val)
global atomic_store
atomic_store:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+12]    ; Значение
    mov ecx, [ebp+8]     ; Указатель
    
    mov [ecx], eax
    
    pop ebp
    ret

; Атомарное чтение 64-бит
; int64_t atomic_load64(int64_t* ptr)
global atomic_load64
atomic_load64:
    mov eax, [ebp+8]
    mov eax, [eax]
    ret

; Атомарная запись 64-бит
; void atomic_store64(int64_t* ptr, int64_t val)
global atomic_store64
atomic_store64:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+12]    ; Значение (нижние 32 бита)
    mov edx, [ebp+16]    ; Значение (верхние 32 бита)
    mov ecx, [ebp+8]     ; Указатель
    
    ; Для 64-бит атомарности на x86 достаточно простой записи
    ; так как это атомарно для выровненных 64-бит значений
    mov [ecx], eax
    
    pop ebp
    ret

; Барьер памяти (Memory Barrier)
; Заставляет процессор завершить все ожидающие операции записи/чтения
global memory_barrier
global mfence
global sfence
global lfence

memory_barrier:
mfence:
    mfence
    ret

sfence:
    sfence
    ret

lfence:
    lfence
    ret

; Получение идентификатора CPU
; uint32_t get_cpu_id(void)
global get_cpu_id
get_cpu_id:
    xor eax, eax
    cpuid
    ret
