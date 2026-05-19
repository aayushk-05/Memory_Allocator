# High-Performance Memory Allocator Suite

> A low-level C++ memory management engine designed to bypass standard OS allocation overhead. 

This repository implements two custom user-space memory allocators from scratch using POSIX `mmap`, pitting them head-to-head against standard `glibc malloc` and Google's production-grade `TCMalloc`.

---

## 🏛️ Architecture 1: The Arena Allocator

The Arena Allocator is designed for **pure speed**. It completely abandons the concept of freeing individual objects, making it the perfect engine for scoped lifetimes (e.g., parsing a file, rendering a single game frame, or handling one HTTP request).

### How It Works (Step-by-Step)

1. **The Big Grab:** On initialization, it asks the OS for one massive, contiguous chunk of virtual memory (e.g., 64MB).
2. **The Bump:** When the application requests memory, the allocator simply takes the current offset pointer, moves it forward by the requested size, and returns the old address.
3. **The Wipe:** Instead of freeing objects individually, the entire arena is "freed" in a single CPU cycle by resetting the offset pointer back to zero.

### Visual Execution Trace
```mermaid
flowchart TD
    A[Initialization] --> B[64MB Memory Pool]
    B --> C{Alloc Request 32B}
    C --> D[Return Pointer at Offset 0]
    D --> E[Bump Offset to 32]
    E --> F{Alloc Request 16B}
    F --> G[Return Pointer at Offset 32]
    G --> H[Bump Offset to 48]
    H --> I[Reset Arena!]
    I --> J[Offset returns to 0]
```
## 🏗️ Architecture: The Segregated Free List

The Segregated Free List is a general-purpose dynamic memory allocator. It minimizes external fragmentation by dividing memory management into distinct size classes ("lanes"), mimicking the core architectural design of production engines like Google's `TCMalloc`.

### How It Works (Step-by-Step)

1. **Size Rounding:** When the application requests a specific allocation size (e.g., 20 bytes), the allocator instantly rounds it up to the nearest power of 2 (32 bytes) to ensure hardware-level alignment.
2. **Lane Array Lookup:** The allocator inspects a static array of pointers, indexing directly into the appropriate size class lane (the 32-byte lane index).
3. **On-Demand Page Refill:** If the target lane's linked list is completely empty, a slow-path allocation is triggered. The engine executes a POSIX `mmap` call to request a raw 4KB page from the OS kernel. It then carves this fresh page into equal 32-byte blocks and weaves an intrusive linked list directly through them.
4. **The Fast-Path Pop:** Once the lane contains blocks, the allocator pops the first available free node off the head of the list in guaranteed $O(1)$ constant time and returns that memory pointer to the application.
5. **Intrusive Recycling:** When the application calls `free()`, the allocator does not return the memory to the operating system. Instead, it inspects a hidden header prefixed right before the user pointer to discover its size class, and prepends the node back onto its original lane stack for immediate, zero-overhead reuse.

### Visual Execution Trace
```mermaid
flowchart TD
    A[Initial State: All Size Lanes Empty] --> B[App Requests Allocation: 20 Bytes]
    B --> C[Round up to 32-Byte Size Class Lane]
    C --> D[Lane Is Empty: Trigger System mmap for 4KB Page]
    D --> E[Carve 4KB Page into Uniform 32B Blocks & Populate Lane Stack]
    E --> F[Pop Head Node & Return Pointer to Application]
    F --> G[App Requests Next Allocation: 10 Bytes]
    G --> H[Round up to 16-Byte Size Class Lane]
    H --> I[Lane Is Empty: Trigger 2nd System mmap for 4KB Page]
    I --> J[Carve Page into Uniform 16B Blocks & Populate Lane Stack]
    J --> K[Pop Head Node & Return Pointer to Application]
    K --> L[App Calls free on the original 32-Byte Block]
    L --> M[Inspect Block Header -> Prepend Node Back into 32B Lane Stack]
