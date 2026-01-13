# ESP32 Memory Management: A Deep Dive

A comprehensive exploration of heap, stack, and static memory management on the ESP32 using ESP-IDF and FreeRTOS.


## 📚 Table of Contents

1. [Overview](#overview)
2. [Memory Types on ESP32](#memory-types-on-esp32)
3. [Understanding the Basics](#understanding-the-basics)
4. [Experiments](#experiments)
5. [Key Takeaways](#key-takeaways)
6. [References](#references)


## Overview

ESP-IDF applications follow common computer architecture patterns for memory allocation:
- **Stack**: Dynamic memory controlled by program flow
- **Heap**: Dynamic memory allocated by function calls
- **Static Memory**: Memory allocated at compile time

Since ESP-IDF is a multi-threaded RTOS environment, each FreeRTOS task maintains its own stack, typically allocated from the heap during task creation.


## Memory Types on ESP32

### DRAM (Data RAM)
The ESP32 contains multiple RAM types, with **DRAM** being the primary memory for heap allocation:
- Connected to the CPU's data bus
- Used to hold data
- Most common type accessed as heap
- All DRAM is single-byte accessible (`MALLOC_CAP_8BIT` capability)

### Memory Allocation Functions

```c
// Standard allocation (uses MALLOC_CAP_DEFAULT)
void *ptr = malloc(size);

// Capability-specific allocation
void *ptr = heap_caps_malloc(size, MALLOC_CAP_8BIT);

// Check available heap
size_t free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
```

Both `malloc()` and `heap_caps_malloc()` allocations can be freed using the standard `free()` function.


## Understanding the Basics

### FreeRTOS Task Stacks

Every FreeRTOS task requires its own **stack memory** for:
- Local variables
- Temporary data
- Function call information

#### Stack Size Specification

```c
xTaskCreate(
    Task1,        // Task function
    "Task1",      // Task name
    2048,         // Stack depth in WORDS (not bytes!)
    NULL,         // Parameters
    1,            // Priority
    NULL          // Task handle
);
```

**Important**: The stack size is in **words**, not bytes. On ESP32 (32-bit), this means:
- `2048` words = `2048 × 4` = **8192 bytes** (8 KB)

#### Monitoring Stack Usage

```c
UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
ESP_LOGI(TAG, "Minimum free stack: %u bytes", high_water);
```

This returns the **minimum free stack** remaining during the task's lifetime—telling you how close you are to overflow.

### Common Stack Problems

1. **Stack Overflow**: Task exceeds allocated stack size
2. **Watchdog Timer Reset**: Task runs too long or gets stuck
3. **Heap Fragmentation**: From frequent task creation/deletion
4. **Insufficient RAM**: Not enough heap for new tasks


## Experiments

### Experiment 1: Task Stack Allocation from Heap

**Objective**: Verify that FreeRTOS task stacks are allocated from the system heap.

```c
static void dummy_task(void *arg) {
    uint8_t stack_buffer[256];
    (void)stack_buffer;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// In app_main():
print_heap("After boot");
xTaskCreate(dummy_task, "dummy_task", 2048, NULL, 5, NULL);
print_heap("After task creation");
```

**Results**:
```
Free heap: 305156 → 302748 bytes (~2408 bytes decrease)
```

**Analysis**:
- FreeRTOS allocated approximately 2408 bytes from heap
- Breakdown: ~2048 bytes for stack + ~300-400 bytes for TCB (Task Control Block) and metadata
- **Conclusion**: Task stacks ARE allocated from the system heap, which limits how many tasks you can create

**Fun fact**: If you define the task function but don't call `xTaskCreate()`, the heap size remains unchanged—functions themselves don't consume heap memory.


### Experiment 2: Dynamic Allocation and Compiler Optimization

**Objective**: Observe heap usage with dynamic allocation and explore compiler optimizations.

```c
void *p1 = malloc(1024);
void *p2 = malloc(2048);
ESP_LOGI(TAG, "Allocated p1=1024 bytes, p2=2048 bytes");
ESP_LOGI(TAG, "p1=%p p2=%p", p1, p2);
print_heap("After malloc");
```

**Results**:
```
Free heap: 302748 → 299668 bytes (~3080 bytes decrease)
Expected: 1024 + 2048 = 3072 bytes
Difference: ~8 bytes for heap bookkeeping overhead
```

**Compiler Optimization Discovery**:

If you comment out the line that uses `p1` and `p2`:
```c
// ESP_LOGI(TAG, "p1=%p p2=%p", p1, p2);  // Commented out
```

**Result**: Heap size doesn't change at all!

**Why?** The compiler performs **dead store elimination**:
- Sees that `malloc()` is called but the pointers are never used
- Concludes the allocation has no observable effect
- **Eliminates the malloc calls entirely** at optimization levels O2 or higher
- This is a standard optimization in ESP-IDF's default build configuration


### Experiment 3: Static vs Stack vs Heap Memory

**Objective**: Compare the three fundamental memory types and their impact on system resources.

#### Part A: Static Memory

```c
static uint8_t static_buf[512];

ESP_LOGI(TAG, "static_buf @ %p", static_buf);
print_heap("After static allocation");
```

**Results**:
```
Address: 0x3ffb2aa4
Heap: 304644 bytes (unchanged)
```

**Memory Region Analysis**:
```
Heap fragments:
Fragment 1: 0x3FFAE6E0 - 0x3FFB0000
Fragment 2: 0x3FFB2EA0 - ...

static_buf @ 0x3FFB2AA4 → Between fragments, NOT in heap
```

**Key Points**:
- Static allocation happens at **compile/link time**
- Memory is in the `.bss` or `.data` section, not the heap
- Lifetime: entire program execution
- **Does not consume heap memory**

#### Part B: Stack Allocation (Static Task Creation)

To safely observe task stack behavior, we used **static task creation**:

```c
#define STACK_SIZE 2048
static StackType_t stack_mem[STACK_SIZE];
static StaticTask_t tcb_mem;

static void stack_task(void *arg) {
    uint8_t stack_buf[128];
    
    ESP_LOGI(TAG, "stack_mem start @ %p", stack_mem);
    ESP_LOGI(TAG, "stack_mem end   @ %p", 
             (void *)((uint8_t *)stack_mem + STACK_SIZE));
    ESP_LOGI(TAG, "stack_buf @ %p", stack_buf);
    
    // Verify stack_buf is within task stack
    if ((void *)stack_buf >= (void *)stack_mem &&
        (void *)stack_buf <= (void *)((uint8_t *)stack_mem + STACK_SIZE)) {
        ESP_LOGI(TAG, "stack_buf is inside task stack ✅");
    }
    
    UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "High water mark: %u bytes", high_water);
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// In app_main():
xTaskCreateStatic(stack_task, "stack_task", STACK_SIZE, NULL, 5, 
                  stack_mem, &tcb_mem);
```

**Results**:
```
Stack start:     0x3ffb2bf8
Stack end:       0x3ffb33f8
stack_buf @:     0x3ffb32d0 ✅ (within stack region)
High water mark: 104 bytes remaining
Heap:            302252 bytes (unchanged from before task creation)
```

**Why Static Task Creation?**

We initially tried dynamic task creation (`xTaskCreate`) and reading stack pointers, but this approach was:
- Tricky and unsafe
- Led to stack corruption and crashes
- Difficult to reliably measure

Static task creation solved these problems by:
- Using pre-allocated memory arrays
- Providing exact, known stack addresses
- Avoiding heap allocation for task stacks
- Enabling safe inspection without system crashes

**Analysis**:
- Local variable `stack_buf` resides **inside the task's stack region**
- Stack grows **downward** (higher addresses → lower addresses)
- High water mark of 104 bytes indicates nearly full but safe usage
- **Heap unchanged** because static allocation doesn't use heap for stack

#### Part C: Heap Allocation

```c
uint8_t *heap_buf = heap_caps_malloc(512, MALLOC_CAP_8BIT);
for (int i = 0; i < 512; i++)
    heap_buf[i] = i % 256;  // Touch memory to ensure allocation
    
ESP_LOGI(TAG, "Heap buffer @ %p", heap_buf);
print_heap("After heap allocation");
```

**Results**:
```
Address: 0x3ffb57b4
Free heap: 302252 → 301736 bytes (516 bytes decrease)
```

**Analysis**: 516 bytes consumed = 512 bytes data + ~4 bytes heap metadata


### Experiment 4: Deliberate Stack Overflow

**Objective**: Trigger and observe stack overflow behavior.

#### Attempt 1: Unused Array
```c
void task_fn(void *arg) {
    uint8_t big_buf[2000];  // Declared but never used
    while (1) {
        // Infinite loop, no yield
    }
}
```

**Result**: Task watchdog triggered (not stack overflow!)
- Compiler may optimize away unused array
- Task never yields, so watchdog fires
- **No stack overflow detected**

#### Attempt 2: Adding Task Yield
```c
void task_fn(void *arg) {
    uint8_t big_buf[2000];
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));  // Yield to allow canary check
    }
}
```

**Result**: Unpredictable behavior
- Sometimes works normally
- Sometimes reboots multiple times
- High water mark shows 724 bytes free (suspicious!)
- Array still not fully utilized by compiler

#### Attempt 3: Reading High Water Mark
```c
void task_fn(void *arg) {
    uint8_t big_buf[2000];
    UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "High water mark: %u bytes", high_water);
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

**Result**: Still unpredictable
- Querying high water mark doesn't force array usage
- Compiler still sees array as unused
- No consistent stack overflow

#### Attempt 4: Forcing Array Usage ✅
```c
void task_fn(void *arg) {
    uint8_t big_buf[2000];
    
    // Force compiler to allocate and use array
    for (int i = 0; i < 2000; i++)
        big_buf[i] = i;
        
    UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "High water mark: %u bytes", high_water);
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Created with insufficient stack:
xTaskCreate(task_fn, "bad_task", 1024, NULL, 5, NULL);
```

**Result**: **Persistent stack overflow errors! ✅**
- Compiler cannot optimize away the array
- Full 2000 bytes allocated on 1024-byte stack
- FreeRTOS **stack canary** detects overflow
- High water mark shows `0` bytes free
- System crashes or resets repeatedly

**Lessons Learned**:
1. Compiler optimizations can mask stack issues during development
2. Always "touch" allocated memory in tests to ensure realistic behavior
3. FreeRTOS stack canary protection works when arrays are actually used
4. Stack overflows cause unpredictable crashes and system instability


### Experiment 5: Memory Pool Implementation

**Objective**: Implement a simple fixed-block memory pool for predictable, deterministic allocation.

#### What is a Memory Pool?

A **memory pool** is a pre-allocated block of memory divided into fixed-size chunks that can be efficiently allocated and freed without using the general-purpose heap. It's essential in embedded and real-time systems for predictable memory behavior.

#### Implementation

```c
// Configuration
#define POOL_SIZE 10   // Number of blocks
#define BLOCK_SIZE 64  // Size of each block in bytes

// Pool storage (static allocation)
static uint8_t pool[POOL_SIZE][BLOCK_SIZE];
static bool used[POOL_SIZE] = {0};  // Track which blocks are in use

// Allocate a block from the pool
void *pool_alloc(void) {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (!used[i]) {
            used[i] = true;
            ESP_LOGI(TAG, "Allocated block %d at %p", i, pool[i]);
            return pool[i];
        }
    }
    ESP_LOGW(TAG, "Memory pool exhausted!");
    return NULL;
}

// Free a block back to the pool
void pool_free(void *ptr) {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (ptr == pool[i]) {
            used[i] = false;
            ESP_LOGI(TAG, "Freed block %d at %p", i, pool[i]);
            return;
        }
    }
    ESP_LOGW(TAG, "Invalid pointer: %p", ptr);
}
```

#### Test Task: UART Message Simulation

```c
void uart_sim_task(void *arg) {
    int msg_count = 0;
    
    while (1) {
        // Simulate receiving a message
        void *buf = pool_alloc();
        if (buf != NULL) {
            // Fill buffer with dummy data
            snprintf((char *)buf, BLOCK_SIZE, "Message #%d", msg_count++);
            ESP_LOGI(TAG, "Received: %s", buf);
            
            // Simulate processing delay
            vTaskDelay(pdMS_TO_TICKS(200));
            
            // Note: pool_free(buf) intentionally commented out
            // to demonstrate pool exhaustion
        } else {
            ESP_LOGW(TAG, "Pool exhausted! Cannot receive message.");
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
```

#### Results

```
Heap before task: 304500 bytes
Heap after task:  302092 bytes (~2408 bytes for task stack/TCB)

Allocated block 0 at 0x3ffb2ab0 → "Message #0"
Allocated block 1 at 0x3ffb2af0 → "Message #1"
Allocated block 2 at 0x3ffb2b30 → "Message #2"
...
Allocated block 9 at 0x3ffb2cf0 → "Message #9"

⚠️ Memory pool exhausted!
⚠️ Pool exhausted! Cannot receive new message.
```

#### Key Observations

1. **Predictable Allocation**:
   - Each block has a fixed, known address
   - Addresses are contiguous and sequential
   - Allocation time is constant (O(n) where n = POOL_SIZE)

2. **No Heap Fragmentation**:
   - Pool is statically allocated at compile time
   - Zero heap consumption for pool allocations
   - Heap decrease (2408 bytes) is only from task creation

3. **Deterministic Capacity**:
   - System can handle exactly 10 concurrent allocations
   - Clear, predictable resource limits
   - No surprise failures from heap fragmentation

4. **Resource Exhaustion Behavior**:
   - Pool exhaustion is explicit and controllable
   - Can implement proper error handling
   - If blocks are freed, memory is immediately reusable

#### Advantages vs malloc()

| Feature | Memory Pool | malloc() |
|---------|-------------|----------|
| Speed | Fast, constant-time | Variable, depends on heap state |
| Determinism | Completely predictable | Unpredictable with fragmentation |
| Fragmentation | None | Can fragment over time |
| Overhead | Minimal (just boolean tracking) | Per-allocation metadata |
| Real-time | Safe for ISRs and RT tasks | Not recommended for ISRs |
| Capacity | Fixed, known upfront | Dynamic, can fail unexpectedly |

#### Trade-offs

**Advantages**:
- ✅ Deterministic allocation time (critical for real-time)
- ✅ No fragmentation
- ✅ Safe in ISRs and driver code
- ✅ Explicit resource limits aid capacity planning
- ✅ Fast recycling of resources

**Limitations**:
- ❌ Less flexible (fixed block size)
- ❌ Limited number of blocks
- ❌ Potential memory waste if blocks are partially used
- ❌ Not suitable for variable-size allocations

#### When to Use Memory Pools

Use memory pools when:
- Working in real-time or embedded systems
- Need predictable allocation times
- Processing similar-sized data repeatedly (e.g., network packets, UART messages)
- Operating in ISRs or critical sections
- Want to avoid heap fragmentation
- Need explicit control over resource limits



## Key Takeaways

### Memory Type Comparison

| Type | Allocation Time | Location | Lifetime | Heap Impact |
|------|----------------|----------|----------|-------------|
| **Static** | Compile time | .data/.bss section | Program lifetime | None |
| **Stack** | Runtime (automatic) | Task stack region | Function scope | Indirect (stack allocated from heap) |
| **Heap** | Runtime (explicit) | Heap region | Until freed | Direct |

### Best Practices

1. **Task Stack Sizing**:
   - Always monitor with `uxTaskGetStackHighWaterMark()`
   - Add 10-20% safety margin beyond measured usage
   - Remember: size is in **words**, not bytes (multiply by 4 for ESP32)

2. **Memory Allocation Strategy**:
   - Use **static allocation** for predictable, permanent data
   - Use **stack** for temporary, small variables
   - Use **heap** sparingly for dynamic, variable-size data
   - Consider **memory pools** for real-time, fixed-size allocations

3. **Avoiding Stack Overflow**:
   - Monitor high water marks during development
   - Touch allocated arrays in tests to prevent compiler optimizations from hiding issues
   - Enable FreeRTOS stack overflow checking
   - Use static task creation for critical, memory-sensitive tasks

4. **Heap Management**:
   - Check return values from `malloc()`/`heap_caps_malloc()`
   - Free allocated memory when done
   - Monitor heap fragmentation with `heap_caps_print_heap_info()`
   - Be aware of compiler optimizations that may eliminate "unused" allocations

5. **Real-Time Considerations**:
   - Memory pools provide deterministic behavior
   - Heap fragmentation can cause unpredictable failures
   - Static allocation eliminates runtime memory concerns
   - Consider worst-case scenarios when sizing resources



## References

- [ESP-IDF Heap Memory Allocation Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/mem_alloc.html)
- [ESP32 FreeRTOS Task Priority and Stack Management](https://controllerstech.com/esp32-freertos-task-priority-stack-management/)
- [Stack, heap and memory discussion on r/esp32](https://www.reddit.com/r/esp32/comments/1hghcmt/stack_heap_and_memory_what_is_what_and_how/)



## Conclusion

Understanding ESP32 memory management is crucial for building reliable embedded systems. Through these experiments, we've explored:

- How FreeRTOS allocates task stacks from the heap
- The differences between static, stack, and heap memory
- Compiler optimizations that can mask memory issues
- How to detect and debug stack overflows
- The benefits of memory pools for deterministic allocation

The key insight: **choose the right tool for the job**. Static allocation for permanent data, stack for temporary variables, heap for dynamic needs, and memory pools for real-time requirements. Always measure, always verify, and plan for worst-case scenarios.

Happy coding! 🚀