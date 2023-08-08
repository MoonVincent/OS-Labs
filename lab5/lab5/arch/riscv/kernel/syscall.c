#include "syscall.h"
#include "proc.h"
#include "stddef.h"
extern struct task_struct* current;


long sys_write(unsigned int fd, const char* buf, size_t count){
    if(fd == 1){
        printk("%s",buf);
    }
    return count;
}

long sys_getpid(){
    return current->pid;
}