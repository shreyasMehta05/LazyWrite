# Copy-on-Write Fork for `xv6` 🐄💻  
![Waketime](https://img.shields.io/badge/Waketime-9%20hrs%2034%20minutes-blueviolet?style=plastic)

This project implements **Copy-on-Write (CoW)** functionality in the `xv6` operating system, enabling processes to:  
- 🛠 **Share memory pages** in a read-only state to minimize duplication.  
- 🚀 **Enhance performance** by copying pages only when modified.  

---

## 👩‍💻 **Key Features**  
- **`PTE_COW` Flag**: Marks pages as Copy-on-Write, triggering page faults on write attempts.  
- **Reference Counting**: Tracks shared memory usage with a `Ref` struct to handle page sharing efficiently.  

---

## ✨ **Implementation Overview**  

### 🔑 **Core Additions**  
1. **`add_the_reference`**: Increases reference count when a page is shared.  
2. **`decreasereferences_and_check`**: Decreases reference count and determines if a page needs duplication.  
3. **`copy_page`**: Duplicates a page’s data when write access occurs.  
4. **`handle_page_fault`**: Detects valid CoW faults and prepares for page duplication.  
5. **`copy_and_remap_page`**: Duplicates and remaps the faulted page in memory.  

### 📝 **Page Fault Tracking**  
- Tracks the number of page faults per process with a **`pagefaultcount`** variable.  
- Logs page fault statistics on process termination.  

---

## 🧪 **Performance Tests**

### ✅ **Read-Only Test (`cow_read_test.c`)**  
**Purpose**: Verify memory efficiency during read-only operations.  
- Parent process initializes memory and forks a child to read the data.  
- **Result**: No unnecessary page duplication or faults.  

```bash
$ cow_read_test
simple: Process 4 had 0 page faults
ok
Process 3 had 3 page faults
```

---

### ✍️ **Write Test (`cow_write_test.c`)**  
**Purpose**: Evaluate CoW behavior when modifying shared pages.  
- Parent process initializes memory and forks a child to write to the data.  
- **Result**: Page duplication occurs only upon write access, ensuring minimal overhead.  

```bash
$ cow_write_test
simple: Process 6 had 1 page faults
ok
Process 5 had 3 page faults
```

---

## 📊 **Key Findings**  

1. **Efficient Memory Sharing**:  
   - CoW avoids unnecessary duplication for read-only operations.  
   - Demonstrated by **0 page faults** during the read test.  

2. **Optimized Writes**:  
   - Memory is copied only when modified, reducing duplication overhead.  
   - Verified with **1 page fault** during write tests.  

---

## 👥 **Authors**  
Team `Lorem Ipsum`:  
- **Shreyas Mehta**  
- **Inesh Dheer**  

This implementation showcases how CoW enhances memory efficiency and improves the overall performance of the `xv6` operating system. 🚀