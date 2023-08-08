//arch/riscv/kernel/proc.c
#include "proc.h"
#include "stdint.h"
#include "string.h"
#include "elf.h"
extern void __dummy();
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
            /*当前segement的起始虚地址，可能并不刚好是一个页面的起始地址，分配页面的时候还需要考虑多余的这部分地址，否则地址转换会失败*/
            uint64 offset = (uint64)(phdr->p_vaddr) - PGROUNDDOWN(phdr->p_vaddr);
            uint64 sz = (phdr->p_memsz + offset) / PGSIZE + 1;
            uint64 tar_addr = alloc_pages(sz);
            //printk("%lx\n",((uint64*)tar_addr)[0]);
            uint64 src_addr = (uint64)(uapp_start) + phdr->p_offset;
            /*在对应位置copy程序内容*/
            memcpy(tar_addr+offset,src_addr,phdr->p_memsz);
            int perm = 0x11 | (phdr->p_flags<<1);

            //va->pa的映射，块头部空白的地址也需要映射，这样才能保证正确（类似于内部碎片）
            create_mapping((uint64*)task->pgd,(uint64)PGROUNDDOWN(phdr->p_vaddr),(uint64)(tar_addr)-PA2VA_OFFSET,sz,perm);
        }
    }

    

    // allocate user stack and do mapping
    uint64 addr = alloc_page();
    create_mapping(task->pgd,(uint64)(USER_END)-PGSIZE,(uint64)(addr-PA2VA_OFFSET),1,23);   //映射用户栈 U-WRV

    task->thread.sepc = ehdr->e_entry;
    //printk("%d\n",ehdr->e_entry);
    task->thread.sstatus = 0x0000000000040020;
    task->thread.sscratch = USER_END;

}


void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle
    idle = (struct task_struct *)kalloc();
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;

    /* YOUR CODE HERE */

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    for(int i=1;i<NR_TASKS;i++){
        task[i] = (struct task_struct *)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;
        task[i]->thread.ra = &__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE;

        task[i]->pgd = alloc_page();

        memcpy(task[i]->pgd,swapper_pg_dir,PGSIZE);

        load_program(task[i]);

        //load_binary_program
        /*task[i]->thread.sepc = USER_START;
        task[i]->thread.sscratch = USER_END;    //user_sp
        task[i]->thread.sstatus = 0x0000000000040020;

        task[i]->thread_info.user_sp = USER_END;
        task[i]->thread_info.kernel_sp = (uint64)task[i] + PGSIZE;
        
        uint64 sz = (uapp_end - uapp_start)/PGSIZE+1;
        uint64 addr = alloc_pages(sz);
        for(uint64 j = 0;j < sz * PGSIZE; ++j){
            ((uint64*)addr)[j] = uapp_start[j];
        }
        create_mapping(task[i]->pgd,(uint64)USER_START,(uint64)(addr-PA2VA_OFFSET),sz,31);    //映射代码段 UXWRV

        addr = alloc_page();
        create_mapping(task[i]->pgd,(uint64)(USER_END)-PGSIZE,(uint64)(addr-PA2VA_OFFSET),1,23);   //映射用户栈 U-WRV*/
    }

    printk("...proc_init done!\n");
    printk("idle process is running!\n");
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
        if(task[i]->counter < min && task[i]->counter > 0){
            min = task[i]->counter;
            min_index = i;
        }
    }
    if(min_index == -1 ){
        for(int i=1;i<NR_TASKS;i++){
            task[i]->counter = rand();
            printk("SET [PID = %d COUNTER = %d]\n",task[i]->pid,task[i]->counter);
        }
        schedule();
    }else{

        printk("switch to [PID = %d COUNTER = %d]\n",task[min_index]->pid,task[min_index]->counter);
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
            task[i]->counter = rand();
            printk("SET [PID = %d PRIORITY = %d]\n",task[i]->pid,task[i]->counter,task[i]->priority);
        }
    }else{
            printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",task[min_index]->pid,task[min_index]->priority,task[min_index]->counter);
            switch_to(task[min_index]);
    }


}

#endif