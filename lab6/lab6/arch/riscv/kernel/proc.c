//arch/riscv/kernel/proc.c
#include "proc.h"
#include "stdint.h"
#include "string.h"
#include "elf.h"
extern void __dummy();
#define PGOFFSET(addr) ((addr) - PGROUNDDOWN(addr))
#define VA2PA(x) ((x - (uint64_t)PA2VA_OFFSET))
#define PA2VA(x) ((x + (uint64_t)PA2VA_OFFSET))
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
extern uint64  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern uint64 uapp_start[];
extern uint64 uapp_end[];
struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

static uint64_t load_program(struct task_struct* task) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)uapp_start; //此时指向elf数据头

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;   //指向数据体
    int phdr_cnt = ehdr->e_phnum;   //segement的metedata数量

    Elf64_Phdr* phdr;
    for (int i = 0; i < phdr_cnt; i++) {    //遍历每一个segement
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);  //获取当前segement的数据指针
        if (phdr->p_type == PT_LOAD) {
            uint64 offset = (uint64)(phdr->p_vaddr) - PGROUNDDOWN(phdr->p_vaddr);
            uint64 sz = (phdr->p_memsz + offset) / PGSIZE + 1;
            uint64 length = sz * PGSIZE;
            uint64 flags = phdr->p_flags << 1; //XWR 右移是将低位置0 表示该页不是一个匿名页
            do_mmap(task,phdr->p_vaddr,length,flags,phdr->p_offset,phdr->p_filesz);
        }
    }

    // allocate user stack and do mapping
    //只进行vma的映射，并不进行读取，当真正访问该页时，才读取对应的页
    do_mmap(task,USER_END-PGSIZE,PGSIZE,VM_R_MASK | VM_W_MASK | VM_ANONYM,0,0); //stack对应的页是一个匿名页
    task->thread.sepc = ehdr->e_entry;
    //while(1);
    
    task->thread.sstatus = 0x0000000000040020;
    task->thread.sscratch = USER_END;

}


void task_init() {

    idle = (struct task_struct *)kalloc();
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;

    task[1] = (struct task_struct *)kalloc();
    task[1]->state = TASK_RUNNING;
    task[1]->counter = 0;
    task[1]->priority = 1;
    task[1]->pid = 1;
    task[1]->thread.ra = &__dummy;
    task[1]->thread.sp = (uint64)task[1] + PGSIZE;
    task[1]->pgd = alloc_page();
    memcpy(task[1]->pgd,swapper_pg_dir,PGSIZE);

    load_program(task[1]);

    printk("[S] Initialized: pid: 1, priority: 1, counter: 0");
}

// arch/riscv/kernel/proc.c

void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            //printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            printk("[PID = %d] is running! thread space begin at 0x%lx\n", current->pid, current);
        }
    }
}

// arch/riscv/kernel/proc.c



void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    if(next->pid != current->pid){
        
        struct task_struct *tmp = current;
        //if(next->pid != 1) while(1);
        current = next;
        __switch_to(tmp,current);

    }
}

void do_timer(){
    //若进程为idle，则直接调度
    if(current->pid == 0){
        schedule();
    }
    else{
        //剩余运行时间--
        current->counter--;
        //剩余运行时间扣完，则调度
        if(current->counter <=0 ){
            schedule();
        }
    }
}

#ifdef SJF

void schedule(){

    uint64 min = 0xFFFFFFFFFFFFFFFF,min_index = -1;
    for(int i=1;i<NR_TASKS;i++){
        if(task[i] != NULL && task[i]->counter < min && task[i]->counter > 0){
            min = task[i]->counter;
            min_index = i;
        }
    }
    if(min_index == -1 ){
        for(int i=1;i<NR_TASKS;i++){
            if(task[i] != NULL){
                task[i]->counter = rand()%10+3;
                task[i]->priority = rand()%10+1;
                printk("[S] Set schedule: pid: %d, priority: %d, counter: %d\n",task[i]->pid,task[i]->priority,task[i]->counter);
            }
        }
        schedule();
    }else{

        printk("[S] Switch to: pid: %d, priority: %d, counter: %d\n",task[min_index]->pid,task[min_index]->priority,task[min_index]->counter);
        switch_to(task[min_index]);
    }
}

#endif

#ifdef PRIORITY

void schedule(){

    int min = 0xFFFFFFFFFFFFFFFF,min_index = -1;
    for(int i=1;i<NR_TASKS;i++){
        if(task[i]->priority < min && task[i]->counter > 0){
            min = task[i]->priority;
            min_index = i;
        }
    }

    if(min_index == -1){
        for(int i=1;i<NR_TASKS;i++){
            task[i]->counter = rand()%10+1;
            task[i]->priority = rand()%10+1;
            printk("[S] Set schedule: pid: %d, priority: %d, counter: %d\n",task[i]->pid,task[i]->priority,task[i]->counter);
        }
    }else{
            printk("[S] Switch to: pid: %d, priority: %d, counter: %d\n",task[min_index]->pid,task[min_index]->priority,task[min_index]->counter);
            switch_to(task[min_index]);
    }


}

#endif

void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
    uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file){
    
    struct vm_area_struct* vma = &(task->vmas[task->vma_cnt]);
    task->vma_cnt++;
    vma->vm_flags = flags;
    vma->vm_content_offset_in_file = vm_content_offset_in_file;
    vma->vm_content_size_in_file = vm_content_size_in_file;
    vma->vm_start = addr;
    vma->vm_end = addr + length;
}

struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr){

    struct vm_area_struct *ret = NULL;
    for(int i = 0; i < task->vma_cnt; ++i){
        if(addr >= task->vmas[i].vm_start && addr < task->vmas[i].vm_end){
            ret = &(task->vmas[i]);
            break;
        }
    }
    return ret;
}
