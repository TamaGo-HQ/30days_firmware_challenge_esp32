# ESP32 Memory Monitor — Mini Project

A small firmware application for **observing heap behavior over time** on ESP32 using FreeRTOS and ESP-IDF.

This project focuses on **visibility** of memory usage rather than optimization.


##  Table of Contents

1. [Overview](#overview)
2. [Objectives](#objectives)
3. [System Architecture](#system-architecture)
4. [Memory Metrics](#memory-metrics)
5. [Implementation Notes](#implementation-notes)
6. [Example Output](#example-output)
7. [Future Extensions](#future-extensions)
8. [References](#references)


## Overview

The ESP32, running FreeRTOS, allocates dynamic memory from the **heap** for tasks, malloc/calloc/realloc calls, and other system resources. Monitoring heap usage is crucial to:

* Detect leaks
* Identify fragmentation
* Understand worst-case memory pressure

This project implements a **periodic memory monitor task** that logs key heap metrics to UART, with simple ASCII bar visualization.


## Objectives

* Track heap usage over time
* Observe fragmentation and minimum free memory
* Understand allocation patterns during task execution
* Produce readable, concise logs for analysis

**Non-goals**:

* Replace ESP-IDF logging
* Implement a production-ready logger
* ISR-safe logging
* Heap/stack optimization


## System Architecture

**Tasks**

| Task Name          | Purpose                                                                      |
| ------------------ | ---------------------------------------------------------------------------- |
| `mem_monitor_task` | Samples heap metrics, prints snapshot every 5 seconds                        |
| `alloc_task`       | Allocates/frees 10 KB blocks periodically to simulate load                   |
| `fragment_task`    | Allocates/free multiple blocks of varying sizes to demonstrate fragmentation |

**Dependencies**

* FreeRTOS (tasks, delays)
* ESP-IDF heap API (`heap_caps_get_free_size`, etc.)
* ESP-IDF logging (`ESP_LOGI`)

**Timing**

* `mem_monitor_task` runs every **5 seconds**
* Other tasks run at their own intervals to simulate dynamic memory usage



## Memory Metrics

### 1. Free Heap

* **API:** `heap_caps_get_free_size(MALLOC_CAP_DEFAULT)`
* **Meaning:** Current unused heap memory
* **Use:** Detect leaks, memory pressure
* **Visualization:** `[■■■■■■____] 62%`

### 2. Minimum Ever Free Heap

* **API:** `heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT)`
* **Meaning:** Historical worst-case free memory
* **Use:** Safety margin, stress indicator
* **Visualization:** `[■■■■______] 41% (min since boot)`

### 3. Largest Free Block

* **API:** `heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)`
* **Meaning:** Largest contiguous allocation possible
* **Use:** Detect fragmentation and allocation failures
* **Visualization:** `[■■■■■_____] 52 KB`



## Implementation Notes

* Memory allocated via `malloc`/`free` always uses the heap.
* Task stacks are allocated from heap during task creation, which reduces free heap.
* Visualization uses **fixed-width ASCII bars** to show percentages intuitively.
* No dynamic allocation occurs inside the monitor task.

**Key design decisions**:

* Keep `mem_monitor_task` lightweight and safe
* Bar width = 10 units for clarity
* Percentages only for meaningful metrics (free heap, min free)
* Largest block shown in KB, no misleading percentage


## Example Output

```
I (30280) MEM_MON: Heap status:
I (30280) MEM_MON:   Free heap       [■■■■■■____] 62%
I (30280) MEM_MON:   Min free (ever) [■■■■______] 41%
I (30280) MEM_MON:   Largest block   [__________] 52 KB
```



## Future Extensions

| Feature                     | Description                                         |
| --------------------------- | --------------------------------------------------- |
| Custom UART driver          | Replace ESP-IDF logging for fully controlled output |
| Custom printf-style logging | Avoid heap usage and reentrancy issues              |
| Stack high-water monitoring | Track task stack usage and detect overflows         |
| Persistent logging          | Save memory trends for later analysis               |
| Visualization tools         | Graphs or remote dashboards                         |

> These are mentioned for awareness; intentionally **out of scope** for this mini-project.



## References

* [ESP-IDF Heap Memory Allocation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/mem_alloc.html)
* [FreeRTOS Stack Monitoring](https://www.freertos.org/uxTaskGetStackHighWaterMark.html)
* [ESP32 Memory Architecture](https://www.espressif.com/en/support/documents/technical-documents)
