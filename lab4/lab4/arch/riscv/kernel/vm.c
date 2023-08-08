// arch/riscv/kernel/vm.c
#include "defs.h"
#include "mm.h"
#include "string.h"
#include "printk.h"

extern uint64 _stext;
extern uint64 _srodata;
extern uint64 _sdata;

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
uint64  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

uint64 OFFSET = PA2VA_OFFSET;

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */

    //PPN[2] = 0x80000000 >> 12 = 0x80000
    uint64 PPN = (uint64)0x80000 << 10;  //0x80000为opensbi起始地址的物理块号 PPN字段从PTE的第10位开始，因此左移10位
    uint64 prot_bit = 0xf;   //VRWX位都为1
    //0x80000000 -> 00......000(high bits 25bits) 000000010(index 9bits) 0000.......000000(offset 30bits)
    early_pgtbl[2] = PPN | prot_bit;
    //0xffffffe0 00000000 ->11......111(high bits 25bits) 110000000(index 9bits) 0000.......000000(offset 30bits)
    early_pgtbl[0x180] = PPN | prot_bit;
    /*printk("%lx\n",early_pgtbl[2]);
    printk("%lx\n",early_pgtbl[0x180]);*/
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
uint64  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir,(uint64)&_stext,(uint64)(&_stext)-PA2VA_OFFSET,2,11);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir,(uint64)&_srodata,(uint64)(&_srodata)-PA2VA_OFFSET,1,3);

    // mapping other memory -|W|R|V
    // 128MB = 128 * 1024 = 131072 kB = 131072 / 4 = 32768 sz
    create_mapping(swapper_pg_dir,(uint64)&_sdata,(uint64)(&_sdata)-PA2VA_OFFSET,32765,7);

    // set satp with swapper_pg_dir
    
    uint64 tmp_satp = 0x8000000000000000;
    //printk("%lx\n",swapper_pg_dir);
    uint64 swapper_p = (uint64)swapper_pg_dir - PA2VA_OFFSET;
    //printk("%lx\n",swapper_p);
    uint64 swapper_ppn = swapper_p >> 12;
    //printk("%lx\n",swapper_ppn);
    tmp_satp += swapper_ppn;
    //printk("%lx\n",tmp_satp);

    asm volatile("csrw satp,%[src]"::[src]"r"(tmp_satp):);
    //printk("SUCCESS\n");


    // flush TLB
    asm volatile("sfence.vma zero, zero");
    //printk("SUCCESSFUL1\n");

    // flush icache
    asm volatile("fence.i");
    //printk("SUCCESSFUL2\n");
    return;
}

/* 创建多级页表映射关系 */
create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */


    //sz的单位是4kb,即sz代表要映射的页面个数
    //判断是leaf page table：pte.v = 1 ,并且pte.r或者pte.x其中一个为1
    for(int i=0;i<sz;i++){
        
        //设置第二级页表
        uint64 vpn_2 = (va >> 30) & 0x1ff;     //va中的30-39位为vpn_2
        uint64 *level2;

        //printk("%d\n",vpn_2);
        if(pgtbl[vpn_2] & 0x1 ){
            uint64 ppn = (pgtbl[vpn_2] >> 10) & 0xfffffffffff;   //左移12位之后取44位 ppn2：26位 ppn1：9位 ppn0：9位
            level2 = ppn << 12;     //始址，低12位的offet为0
            //printk("a:%lx\n",level2);
        }else{
            level2 = kalloc() - PA2VA_OFFSET;   //新申请一个页用于存储页表

            //printk("b:%lx\n",level2);
            pgtbl[vpn_2] = (uint64)level2 >> 2; //地址中的ppn位于12-55位，pte中的ppn位于10-53位
            pgtbl[vpn_2] |= 0x1;
        }

        //设置第三级页表
        uint64 vpn_1 = (va >> 21) & 0x1ff;
        uint64 *level3;

        if(level2[vpn_1] & 0x1 ){
            uint64 ppn = (level2[vpn_1] >> 10) & 0xfffffffffff;   //左移12位之后取44位 ppn2：26位 ppn1：9位 ppn0：9位
            level3 = ppn << 12;
            //printk("c:%lx\n",level3);
        }else{
            level3 = kalloc() - PA2VA_OFFSET;
            //printk("d:%lx\n",level3);
            level2[vpn_1] = (uint64)level3 >> 2;
            level2[vpn_1] |= 0x1;
        }

        //第三级页表映射到物理页
        uint64 vpn_0 = (va >> 12) & 0x1ff;
        if( !(level3[vpn_0] & 0x1) ){
            level3[vpn_0] = (uint64)pa >> 2;
            level3[vpn_0] |= perm;
        }
        //0x1000 B = 16^3 B = 2^12 B = 4kB,即一个页面的大小
        va = va + 0x1000;
        pa = pa + 0x1000;
    }
    //printk("end\n");
}

