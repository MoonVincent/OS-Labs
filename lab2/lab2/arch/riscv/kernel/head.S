.extern start_kernel

    .section .text.init
    .globl _start
_start:

    la sp,boot_stack_top
    call mm_init
    call task_init
    #-------------------------
    	#set stvec = _traps
    #-------------------------
    
    la t0,trap_s
    csrw stvec,t0
    
    #-------------------------
    	#set sie[STIE] = 1
    #-------------------------
    
    csrr t0,sie
    ori t0,t0,32
    csrw sie,t0
    
    #-------------------------
    	#set first time interrupt
    #-------------------------
    
    add a7,x0,x0
    add a6,x0,x0
    li t0,10000000
    rdtime a0
    add a0,a0,t0
    add a1,x0,x0
    add a2,x0,x0
    add a3,x0,x0
    add a4,x0,x0
    add a5,x0,x0
    ecall
    
    #-------------------------
    	#set sstatus[SIE] = 1
    #-------------------------
    
    csrr t0,sstatus
    ori t0,t0,2
    csrw sstatus,t0

    
    #-------------------------
    	#lab1 code
    #-------------------------
    call start_kernel

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size
    .globl boot_stack_top
boot_stack_top:
