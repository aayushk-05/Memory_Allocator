
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

### Visual Flow
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
