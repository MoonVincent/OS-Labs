#include "syscall.h"
#include "proc.h"
extern uint64 uapp_start[];
extern uint64 uapp_end[];
extern struct task_struct* current;
extern struct task_struct* task[NR_TASKS];

void trap_handler(unsigned long scause, unsigned long sepc,struct pt_regs *regs) {
    
    if(scause == 0x8000000000000005){   //time interrupt
        printk("[S] Supervisor Mode Timer Interrupt\n");
        clock_set_next_event();
        do_timer();
    }else if(scause == 0x0000000000000008){ //ecall-from-user-mode
        if(regs->a7 == SYS_WRITE){
            regs->a0 = sys_write(regs->a0,regs->a1,regs->a2);
        }else if(regs->a7 == SYS_GETPID){   //a7
            regs->a0 = sys_getpid();
        }else if(regs->a7 == SYS_CLONE){
            regs->a0 = sys_clone(regs);
        }
        regs->sepc += 4;
    }else if(scause == 0x000000000000000C || scause == 0x000000000000000D || scause == 0x000000000000000F){ //Instruction Page Fault
        printk("[S] Supervisor Page Fault, scause: %lx, stval: %lx, sepc: %lx\n",scause,regs->stval,regs->sepc);
        do_page_fault(regs);
    }else{
        printk("[S] Unhandled trap, ");
        printk("scause: %lx, ", scause);
        printk("stval: %lx, ", regs->stval);
        printk("sepc: %lx\n", regs->sepc);
        while (1);
    }
}


void do_page_fault(struct pt_regs *regs) {
    
    uint64 bad_addr = regs->stval;  //获得访问出错的虚拟内存地址
    struct vm_area_struct *vma = find_vma(current,bad_addr);    //查找bad_addr是否在某个vma中
    if(vma != NULL){    //找到了bad_addr所在的vma ；bad_addr不在vma中的trap处理本次实验不涉及
        uint64 new_addr = alloc_page();
        // X|R|W|V
        create_mapping(current->pgd,PGROUNDDOWN(bad_addr),(uint64)new_addr-PA2VA_OFFSET,1,(vma->vm_flags & (~(uint64_t)VM_ANONYM)) | 0x11);
        if( !(vma->vm_flags & VM_ANONYM) ){ //非匿名page，需要拷贝
            uint64 src_addr = (uint64)(uapp_start) + vma->vm_content_offset_in_file;
            if(bad_addr - vma->vm_start < PGSIZE){
                uint64 offset = (uint64)(bad_addr) - PGROUNDDOWN(bad_addr);
                uint64 sz1 = PGSIZE - offset;
                uint64 sz2 = vma->vm_end - vma->vm_start;
                uint64 sz = sz1 < sz2 ? sz1 : sz2;
                memcpy(new_addr+offset,src_addr,sz);
            }else{
                uint64 sz1 = PGSIZE;
                uint64 sz2 = vma->vm_end - bad_addr;
                uint64 sz = sz1 < sz2 ? sz1 : sz2;
                memcpy(new_addr,src_addr,sz);
            }
        }   //如果是匿名page（如stack），则不需要拷贝
    }
}

