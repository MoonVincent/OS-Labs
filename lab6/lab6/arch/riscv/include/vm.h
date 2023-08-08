#pragma once
create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
int IsMapped(pagetable_t ptb,uint64 addr);