#include "printk.h"
#include "sbi.h"

extern void test();

int start_kernel() {

    printk("[S] 2022 Hello RISC-V\n");

    schedule();

    test();

	return 0;
}
