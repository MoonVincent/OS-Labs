//arch/riscv/kernel/proc.c
#include "proc.h"
extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

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