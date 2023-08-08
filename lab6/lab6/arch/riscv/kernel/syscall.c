#include "syscall.h"
#include "proc.h"
#include "stddef.h"
extern struct task_struct* current;
extern void __ret_from_fork();
extern struct task_struct* current;        // 指向当前运行线程的 `task_struct`
extern struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此
extern uint64  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

long sys_write(unsigned int fd, const char* buf, size_t count){
    if(fd == 1){
        printk("%s",buf);
    }
    return count;
}

long sys_getpid(){
    return current->pid;
}

uint64_t sys_clone(struct pt_regs *regs) {
    /*
     1. 参考 task_init 创建一个新的 task, 将的 parent task 的整个页复制到新创建的 
        task_struct 页上(这一步复制了哪些东西?）。将 thread.ra 设置为 
        __ret_from_fork, 并正确设置 thread.sp
        (仔细想想，这个应该设置成什么值?可以根据 child task 的返回路径来倒推)

     2. 利用参数 regs 来计算出 child task 的对应的 pt_regs 的地址，
        并将其中的 a0, sp, sepc 设置成正确的值(为什么还要设置 sp?)

     3. 为 child task 申请 user stack, 并将 parent task 的 user stack 
        数据复制到其中。 

     3.1. 同时将子 task 的 user stack 的地址保存在 thread_info->
        user_sp 中，如果你已经去掉了 thread_info，那么无需执行这一步

     4. 为 child task 分配一个根页表，并仿照 setup_vm_final 来创建内核空间的映射

     5. 根据 parent task 的页表和 vma 来分配并拷贝 child task 在用户态会用到的内存

     6. 返回子 task 的 pid
    */

    //1 创建一个新的task
    struct task_struct *child = (struct task_struct*)kalloc();
    memcpy((void*)child,(void*)current,PGSIZE);
    child->thread.ra = &__ret_from_fork;
    int i;
    for(i = 1;i < 16; i++){
        if(task[i] == NULL)
            break;
    }
    child->pid = i;
    task[i] = child;

    //2 计算出child_task对应的pt_regs的地址
    //因为pt_regs与task的其它变量存储在同一个page中，因此可以算出regs在一页中的offset而得知其地址
    //uint64 offet = PGOFFSET((uint64)regs);
    uint64 regs_int = (uint64)regs;
    uint64 offset = regs_int - PGROUNDDOWN(regs_int);
    //child为当前task分配的物理页的起始地址，加上计算出的offset就可以计算出pt_regs的地址
    struct pt_regs *child_regs = (struct pt_regs*)(child + offset);
    //child->thread.sp = (uint64_t)child_regs;    //进trap_handler时，传入的regs即等于父进程的sp，此处子进程行为要保持一致
    child->thread.sp = (uint64)child_regs;
    memcpy((void*)child_regs,(void*)regs,28*8);

    child_regs->a0 = 0; //子进程的fork返回值为0，父进程的fork返回值为子进程的pid
    child_regs->sp = (uint64)child_regs;    //设置pt_regs中的sp值，这样在从fork中返回时，可以成功恢复各个寄存器的值
    child_regs->sepc = regs->sepc + 4;  //fork后，子进程执行的指令为父进程的下一条
    //3. 为 child task 申请 user stack, 并将 parent task 的 user stack 数据复制到其中。 
    //3.1. 同时将子 task 的 user stack 的地址保存在 thread_info->user_sp 中
    //4 为child task分配一个根页表,并将parent task的根页表复制至child task

    //上述三步可以通过直接复制parent task的页表进行完成
    //为child分配用户栈，并拷贝数据
    uint64 stack = alloc_page();
    memcpy(stack,USER_END-PGSIZE,PGSIZE);

    child->pgd = alloc_page();
    memset(child->pgd, 0x0, PGSIZE);
    memcpy(child->pgd,swapper_pg_dir,PGSIZE);

    // U|X|W|R|V
    create_mapping(child->pgd,USER_END-PGSIZE,(uint64)stack-PA2VA_OFFSET,1,0x17);


    //5 根据parent task的页表和vma来分配并拷贝child task在用户态会用到的内存
    for (int j = 0; j < current->vma_cnt; j++) {
        struct vm_area_struct *vma = &(current->vmas[j]);
        uint64_t cur_addr = vma->vm_start;
        while (cur_addr < vma->vm_end) {
            if (IsMapped(current->pgd, PGROUNDDOWN(cur_addr))) {    //当前地址对应的页面已被parent task调入内存，才进行拷贝
                
                uint64_t page = alloc_page();   //为child task分配一个新的页面
                //将该页面进行映射
                //XWRV
                create_mapping((uint64)child->pgd, PGROUNDDOWN(cur_addr), (uint64)page-PA2VA_OFFSET, 1, (vma->vm_flags & (~(uint64_t)VM_ANONYM)) | 0x11);
                //将parent task中对应页面的内容进行拷贝
                memcpy((void*)page, (void*)PGROUNDDOWN(cur_addr), PGSIZE); // using kernel virtual address to copy
            }
            cur_addr += PGSIZE;
        }
    }

    //6 返回子task的pid
    printk("[S] New task: %d\n", i);
    return i;


}
