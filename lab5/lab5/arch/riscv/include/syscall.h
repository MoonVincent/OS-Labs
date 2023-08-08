#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define SYS_WRITE 64
#define SYS_GETPID 172

#include "stddef.h"

long sys_write(unsigned int fd, const char* buf, size_t count);

long sys_getpid();

#endif