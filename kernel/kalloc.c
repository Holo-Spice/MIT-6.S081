// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PA2PGREF_ID(p)(((p)-KERNBASE)/PGSIZE) // 计算物理地址 p 对应的页框号
#define PGREF_MAX_ENTRIES PA2PGREF_ID(PHYSTOP) // 物理内存中页框的总数

int pageref[PGREF_MAX_ENTRIES]; // 页引用计数数组
struct spinlock pgreflock;      // 页引用计数锁

#define PA2PGREF(p) pageref[PA2PGREF_ID((uint64)(p))] // 获取物理地址 p 对应页框的引用计数

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

void kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&pgreflock, "pgref");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&pgreflock);
  if (--PA2PGREF(pa) <= 0)
  {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run *)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&pgreflock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r)
  {
    memset((char *)r, 5, PGSIZE); // fill with junk
    PA2PGREF(r) = 1;              // 新分配的页引用计数初始化为 1
  }
    return (void *)r;
}

// 增加物理页的引用计数
void krefpage(void *pa)
{
  acquire(&pgreflock);
  PA2PGREF(pa)++;
  release(&pgreflock);
}

// 写时复制一个新的物理地址返回
// 引用数》1 则-1并返回一个新的物理页地址
// 引用数《=1 则直接返回原物理页地址
void *kcopy_n_deref(void *pa)
{
  acquire(&pgreflock);

  if (PA2PGREF(pa) <= 1)
  {
    release(&pgreflock);
    return pa;
  }

  uint64 newpa = (uint64)kalloc();
  // 内存不足，无法分配新页
  if (newpa == 0)
  {
    release(&pgreflock);
    return 0;
  }
  memmove((void *)newpa, (void *)pa, PGSIZE); // 复制原物理页内容到新页
  PA2PGREF(pa)--;                             // 原物理页引用计数减1

  release(&pgreflock);
  return (void *)newpa;
}