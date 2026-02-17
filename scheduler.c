// scheduler.c — Completely Fair Scheduler FIXED
#include "kernel.h"

#define SCHED_LATENCY       20
#define MIN_GRANULARITY     2
#define NICE_0_LOAD         1024

#define TASK_RUNNING        0
#define TASK_SLEEPING       1
#define TASK_BLOCKED        2

struct sched_entity {
    uint64_t vruntime;
    uint64_t exec_start;
    uint64_t sum_exec;
};

// Note: struct thread and struct process defined in kernel.h

struct runqueue {
    struct thread* head;
    struct thread* tail;
    uint32_t nr_running;
    uint64_t min_vruntime;
    spinlock_t lock;
};

static struct runqueue runqueues[MAX_CPUS];
static uint32_t next_tid = 1;
static uint32_t next_pid = 1;

void scheduler_init(void) {
    for (int i = 0; i < MAX_CPUS; i++) {
        spin_init(&runqueues[i].lock);
        runqueues[i].head = NULL;
        runqueues[i].tail = NULL;
        runqueues[i].nr_running = 0;
        runqueues[i].min_vruntime = 0;
    }
    kprintf("Scheduler: CFS initialized\n");
}

static void enqueue(struct runqueue* rq, struct thread* t) {
    t->state = TASK_RUNNING;
    if (!rq->head) {
        rq->head = rq->tail = t;
        t->next = NULL;
    } else {
        // Simple: add to tail (real CFS uses rbtree)
        rq->tail->next = t;
        rq->tail = t;
        t->next = NULL;
    }
    rq->nr_running++;
}

static struct thread* dequeue(struct runqueue* rq) {
    if (!rq->head) return NULL;
    
    struct thread* t = rq->head;
    rq->head = t->next;
    
    if (!rq->head) rq->tail = NULL;
    rq->nr_running--;
    t->next = NULL;
    
    return t;
}

void schedule(void) {
    struct cpu* c = cpu_get_current();
    struct runqueue* rq = &runqueues[c->acpi_id];
    struct thread* prev = (struct thread*)c->current_thread;
    
    spin_lock(&rq->lock);
    
    if (prev && prev->state == TASK_RUNNING) {
        enqueue(rq, prev);
    }
    
    struct thread* next = dequeue(rq);
    
    if (!next) {
        // Idle - no runnable threads
        spin_unlock(&rq->lock);
        return;
    }
    
    c->current_thread = (uint64_t)next;
    
    if (prev && prev != next) {
        tss_set_rsp0(next->kernel_stack);
        context_switch(prev ? &prev->rsp : NULL, next->rsp,
                      next->parent ? next->parent->cr3 : read_cr3());
    }
    
    spin_unlock(&rq->lock);
}

void yield(void) {
    schedule();
}

void sleep(uint64_t ms) {
    // Simple sleep - just yield for now
    // Real implementation would use timer interrupts
    (void)ms;
    yield();
}

void scheduler_ap_entry(void) {
    struct cpu* c = cpu_get_current();
    
    // Create idle thread
    struct thread* idle = (struct thread*)kmalloc(sizeof(struct thread));
    if (!idle) {
        while (1) hlt();
    }
    
    memset(idle, 0, sizeof(struct thread));
    idle->tid = 0;
    idle->state = TASK_RUNNING;
    idle->kernel_stack = (uint64_t)pmm_alloc_pages(4) + 0x10000;
    idle->rsp = idle->kernel_stack;
    idle->parent = NULL;
    
    c->current_thread = (uint64_t)idle;
    
    // Setup timer
    lapic_timer_set_handler(schedule);
    lapic_timer_start_periodic(10); // 10ms ticks
    
    while (1) {
        schedule();
        hlt();
    }
}

struct process* process_create(const char* name, void* entry) {
    struct process* p = (struct process*)kzalloc(sizeof(struct process));
    if (!p) return NULL;
    
    p->pid = next_pid++;
    strncpy(p->name, name, 31);
    p->name[31] = 0;
    
    p->cr3 = vmm_create_address_space();
    
    struct thread* t = (struct thread*)kzalloc(sizeof(struct thread));
    if (!t) {
        kfree(p);
        return NULL;
    }
    
    t->tid = next_tid++;
    t->state = TASK_RUNNING;
    t->prio = 128;
    t->parent = p;
    t->kernel_stack = (uint64_t)pmm_alloc_pages(4) + 0x10000;
    t->rsp = t->kernel_stack - 8;
    
    // Setup initial stack frame for return to entry point
    uint64_t* sp = (uint64_t*)t->rsp;
    *(--sp) = 0;           // Return address (will halt)
    *(--sp) = (uint64_t)entry;  // Entry point
    *(--sp) = 0;           // RBP
    *(--sp) = 0;           // RBX
    *(--sp) = 0;           // R12
    *(--sp) = 0;           // R13
    *(--sp) = 0;           // R14
    *(--sp) = 0;           // R15
    t->rsp = (uint64_t)sp;
    
    p->main_thread = *t;
    
    // Enqueue on BSP (CPU 0)
    enqueue(&runqueues[0], t);
    
    kprintf("Process: %s (PID %u)\n", name, p->pid);
    
    return p;
}

void context_switch(uint64_t* old_rsp, uint64_t new_rsp, uint64_t new_cr3) {
    if (old_rsp) {
        asm volatile ("mov %%rsp, %0" : "=r"(*old_rsp));
    }
    
    if (new_cr3 && new_cr3 != read_cr3()) {
        write_cr3(new_cr3);
    }
    
    asm volatile ("mov %0, %%rsp" : : "r"(new_rsp));
}
