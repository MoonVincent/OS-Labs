.extern start_kernel

    .section .text.init
    .globl _start
_start:

    la sp,boot_stack_top

    call setup_vm

    call relocate

    call mm_init

    call setup_vm_final

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
.extern PA2VA_OFFSET

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    la t2,OFFSET
    ld t2,0(t2)
    add ra,ra,t2
    add sp,sp,t2

    ###################### 
    #   YOUR CODE HERE   #
    ######################

    # set satp with early_pgtbl

    addi t0,x0,1
    slli t0,t0,63   #0x8000000000000000
    la t1,early_pgtbl
    #sub t1,t1,t2
    srli t1,t1,12   #  Get PPN
    add t0,t0,t1
    csrw satp,t0


    ###################### 
    #   YOUR CODE HERE   #
    ######################

    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i

    ret


    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size
    .globl boot_stack_top
boot_stack_top:
