// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"


void* copy_page(void* pa);
void acquireReferenceLock();
void releaseReferenceLock();
int decreasereferences_and_check(void* pa);
int handle_page_fault(pagetable_t pagetable, uint64 va);
int copy_and_remap_page(pagetable_t pagetable, uint64 va, pte_t *pte);


void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct {
  struct spinlock lock;
  int references[MAXREFERENCES];
} Ref;
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&Ref.lock, "Ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
kfree(void *pa)
{
  struct run *r;

  int ref = PA2REF(pa);
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&Ref.lock);
  if(--Ref.references[ref] > 0){
    release(&Ref.lock);
    return;
  }
  release(&Ref.lock);

  memset(pa, 1, PGSIZE);   // Fill with junk to catch dangling refs.
  r = (struct run*)pa;

  acquire(&kmem.lock);

  r->next = kmem.freelist;
  kmem.freelist = r;

  release(&kmem.lock);
}

void acquireReferenceLock() {
  acquire(&Ref.lock);
}

void releaseReferenceLock() {
  release(&Ref.lock);
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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 69, PGSIZE); // fill with junk
    Ref.references[PA2REF((uint64)r)] = 1;
  } 
  return (void*)r;
}

void add_the_reference(void* pa) {
  int reference = PA2REF(pa);
  if (reference < 0 || reference >= MAXREFERENCES)
    return;

  acquireReferenceLock();
  Ref.references[reference]++;
  releaseReferenceLock();
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

// Decrease reference count and return if it needs copying
int decreasereferences_and_check(void* pa) {
  int reference = PA2REF(pa);
  if (reference < 0 || reference >= MAXREFERENCES)
    return -1;  // Invalid reference

  acquireReferenceLock();
  if (Ref.references[reference] <= 1) {
    releaseReferenceLock();
    return 0;  // No need to copy, reference count is 1 or less
  }

  Ref.references[reference]--;
  releaseReferenceLock();
  return 1;  // Reference count decreased, need to copy
}

// Copy the page to another memory address and return the new address
void* copy_page(void* pa) {
  int reference = PA2REF(pa);
  if (reference < 0 || reference >= MAXREFERENCES)
    return 0;

  uint64 mem = (uint64)kalloc();
  if (mem == 0)
    return 0;  // Allocation failed

  memmove((void*)mem, (void*)pa, PGSIZE);
  return (void*)mem;
}

void* copyanddecreasereferences(void* pa) {
  if (decreasereferences_and_check(pa) == 0) {
    return pa;  // No need to copy if reference count was 1 or less
  }
  
  return copy_page(pa);  // Copy the page if reference count was greater than 1
}

// Handle the page fault logic, check conditions, and return if it's a valid COW fault

int handle_page_fault(pagetable_t pagetable, uint64 va) {
  if (va >= MAXVA || va <= 0)
    return -1;

  pte_t *pte = walk(pagetable, va, 0);  // Initialize pte here
  if (pte == 0)
    return -1;
  if (!(*pte & PTE_V) || !(*pte & PTE_U) || !(*pte & PTE_COW))
    return -1;

  myproc()->pagefaultcount++;

  return 0;  // Valid COW fault, proceed to copy and remap
}

// Copy the page to a new memory location and remap it in the page table
int copy_and_remap_page(pagetable_t pagetable, uint64 va, pte_t *pte) {
  uint64 pa = PTE2PA(*pte);
  char* mem = kalloc();
  if (mem == 0)
    return -1;
  
  memmove((void*)mem, (void*)pa, PGSIZE);

  uint64 flags = (PTE_FLAGS(*pte) | PTE_W) & ~PTE_COW;
  uvmunmap(pagetable, PGROUNDDOWN(va), 1, 1);
  if (mappages(pagetable, va, 1, (uint64)mem, flags) == -1) {
    panic("Pagefaulthandler mappages");
  }

  return 0;
}

int pagefaulthandler(pagetable_t pagetable, uint64 va) {
  // Don't pass an uninitialized pte, handle initialization in handle_page_fault
  if (handle_page_fault(pagetable, va) == -1) {
    return -1;  // If the fault is not valid, return -1
  }


  pte_t *pte = walk(pagetable, va, 0);  // Initialize pte here as well
  return copy_and_remap_page(pagetable, va, pte);  // Copy and remap the page
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
