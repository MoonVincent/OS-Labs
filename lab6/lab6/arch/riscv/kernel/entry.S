.section .text.entry
.align 2
.globl trap_s
.globl __dummy
.globl __switch_to

trap_s:
#--------------------
        # 1.save 32 registers and sepc to stack
    #--------------------

        #S-mode -> S-mode 不需要切换栈
        #U-mode -> S-mode 需要切换栈

        csrr t0,sstatus
        addi t1,x0,1
        slli t1,t1,8
        and t2,t1,t0
        bne t2,x0,LABEL0 # SPP != 0 S->mode -> S->mode 不需要切换
        csrr t0,sscratch
        csrw sscratch,sp
        add sp,t0,x0

LABEL0:
        addi sp,sp,-224
        sd sp,0(sp)
        sd ra,8(sp)
        sd gp,16(sp)
        sd tp,24(sp)
        sd s0,32(sp)
        sd s1,40(sp)
        sd a0,48(sp)
        sd a1,56(sp)
        sd a2,64(sp)
        sd a3,72(sp)
        sd a4,80(sp)
        sd a5,88(sp)
        sd a6,96(sp)
        sd a7,104(sp)
        sd s2,112(sp)
        sd s3,120(sp)
        sd s4,128(sp)
        sd s5,136(sp)
        sd s6,144(sp)
        sd s7,152(sp)
        sd s8,160(sp)
        sd s9,168(sp)
        sd s10,176(sp)
        sd s11,184(sp)
        csrr t0,sepc
        sd t0,192(sp)
        csrr t0,stval
        sd t0,200(sp)
        csrr t0,sscratch
        sd t0,208(sp)
        csrr t0,sstatus
        sd t0,216(sp)

    #--------------------
        # 2.call trap_handler
    #--------------------

        csrr a0,scause
        csrr a1,sepc
        add a2,sp,x0
        call trap_handler
        .global __ret_from_fork
__ret_from_fork:

    #--------------------
        # 3.restore sepc and 32 registers (x2(sp) should be restore last) from stack
    #--------------------
        ld t0,216(sp)
        csrw sstatus,t0
        ld t0,208(sp)
        csrw sscratch,t0
        ld t0,200(sp)
        csrw stval,t0
        ld t0,192(sp)
        csrw sepc,t0
        ld s11,184(sp)
        ld s10,176(sp)
        ld s9,168(sp)
        ld s8,160(sp)
        ld s7,152(sp)
        ld s6,144(sp)
        ld s5,136(sp)
        ld s4,128(sp)
        ld s3,120(sp)
        ld s2,112(sp)
        ld a7,104(sp)
        ld a6,96(sp)
        ld a5,88(sp)
        ld a4,80(sp)
        ld a3,72(sp)
        ld a2,64(sp)
        ld a1,56(sp)
        ld a0,48(sp)
        ld s1,40(sp)
        ld s0,32(sp)
        ld tp,24(sp)
        ld gp,16(sp)
        ld ra,8(sp)
        ld sp,0(sp)
        addi sp,sp,224
    #--------------------
        # 4.return from trap
    #--------------------
        #S-mode -> S-mode 不需要切换栈
        #S-mode -> U-mode 需要切换栈

        csrr t0,sstatus
        addi t1,x0,1
        slli t1,t1,8
        and t2,t1,t0
        bne t2,x0,LABEL1 # SPP == 1 S->mode -> S->mode 不需要切换
        csrr t0,sscratch
        csrw sscratch,sp
        add sp,t0,x0

	LABEL1:
        sret

__dummy:
    #S-mode -> U-mode sp此时为S-stack sp
    csrr t0,sscratch
    csrw sscratch,sp    #sscratch存储S-stack sp
    add sp,t0,x0    #S-mode -> U-mode
    sret

__switch_to:
    # save state to prev process
    # YOUR CODE HERE
    addi t0,a0,48
    sd ra,0(t0)
    sd sp,8(t0)
    sd s0,16(t0)
    sd s1,24(t0)
    sd s2,32(t0)
    sd s3,40(t0)
    sd s4,48(t0)
    sd s5,56(t0)
    sd s6,64(t0)
    sd s7,72(t0)
    sd s8,80(t0)
    sd s9,88(t0)
    sd s10,96(t0)
    sd s11,104(t0)
    csrr t1,sepc
    sd t1,112(t0)
    csrr t1,sstatus
    sd t1,120(t0)
    csrr t1,sscratch
    sd t1,128(t0)


    # restore state from next process
    # YOUR CODE HERE
    addi t0,a1,48
    ld ra,0(t0)
    ld sp,8(t0)
    ld s0,16(t0)
    ld s1,24(t0)
    ld s2,32(t0)
    ld s3,40(t0)
    ld s4,48(t0)
    ld s5,56(t0)
    ld s6,64(t0)
    ld s7,72(t0)
    ld s8,80(t0)
    ld s9,88(t0)
    ld s10,96(t0)
    ld s11,104(t0)
    ld t1,112(t0)
    csrw sepc,t1
    ld t1,120(t0)
    csrw sstatus,t1
    ld t1,128(t0)
    csrw sscratch,t1

    #切换页表，由于allo_page()返回的是虚拟地址，所以需要减去OFFSET转化为物理地址
    addi t1,x0,1
    slli t1,t1,63   #0x8000000000000000
    ld t2,136(t0)
    la t3,OFFSET
    ld t3,0(t3)
    sub t2,t2,t3
    srli t2,t2,12   #  Get PPN
    add t2,t2,t1
    csrw satp,t2

    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i

    ret