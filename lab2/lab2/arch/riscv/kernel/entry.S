.section .text.entry
.align 2
.globl trap_s
.globl __dummy
.globl __switch_to

trap_s:
#--------------------
        # 1.save 32 registers and sepc to stack
    #--------------------
        addi sp,sp,-200
        sd sp,0(sp)
        sd ra,8(sp)
        sd gp,16(sp)
        sd tp,24(sp)
        sd fp,32(sp)
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

    #--------------------
        # 2.call trap_handler
    #--------------------
        csrr a0,scause
        csrr a1,sepc
        call trap_handler
    #--------------------
        # 3.restore sepc and 32 registers (x2(sp) should be restore last) from stack
    #--------------------
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
        ld fp,32(sp)
        ld tp,24(sp)
        ld gp,16(sp)
        ld ra,8(sp)
        ld sp,0(sp)
        addi sp,sp,200
    #--------------------
        # 4.return from trap
    #--------------------
	    sret

__dummy:
    la t0,dummy
    csrw sepc,t0
    sret

__switch_to:
    # save state to prev process
    # YOUR CODE HERE
    addi t0,a0,40
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


    # restore state from next process
    # YOUR CODE HERE
    addi t0,a1,40
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

    ret


