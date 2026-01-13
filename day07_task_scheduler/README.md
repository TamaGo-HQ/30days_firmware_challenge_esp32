# FreeRTOS Task Scheduling on ESP32: A Practical Guide

## Overview

This guide explores **FreeRTOS task scheduling** on the ESP32 using ESP-IDF. Through hands-on experiments, we'll observe how the scheduler manages multiple tasks, handles priorities, and leverages the ESP32's dual-core architecture.

## What is FreeRTOS?

FreeRTOS is a **lightweight real-time operating system (RTOS)** designed for microcontrollers. It enables your ESP32 to run multiple independent tasks concurrently without blocking each other. The scheduler manages task execution based on priority and timing requirements, making it ideal for IoT projects, robotics, and embedded systems where multiple processes must run simultaneously.

### Key RTOS Features

- **Multitasking:** Run multiple tasks "simultaneously" on a single CPU
- **Task Priorities:** Assign priorities so critical operations run first
- **Deterministic Behavior:** Tasks execute predictably within known time frames
- **Resource Sharing:** Use semaphores and mutexes to prevent conflicts

On ESP32, FreeRTOS comes **pre-integrated with ESP-IDF**, so you can start creating tasks immediately without worrying about low-level timing management.



## Understanding Tasks

In FreeRTOS, **tasks** are independent functions that run in parallel, each performing a specific job. You might have one task reading sensor data, another sending data to the cloud, and another controlling an LED.

### Task States

Each task exists in one of these states:

- **Running:** Currently using the CPU
- **Ready:** Waiting for CPU time
- **Blocked:** Waiting for a delay or event (semaphore, queue message)
- **Suspended:** Paused until explicitly resumed
- **Deleted:** Removed from memory

### Creating Tasks

Use `xTaskCreate()` to create a task:

```c
BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode,       // Task function pointer
    const char * const pcName,       // Task name (for debugging)
    const uint32_t usStackDepth,     // Stack size in words (1 word = 4 bytes on ESP32)
    void *pvParameters,              // Optional parameters
    UBaseType_t uxPriority,          // Task priority (0 = lowest)
    TaskHandle_t *pvCreatedTask      // Task handle (optional)
);
```

For dual-core ESP32, you can pin tasks to specific cores using `xTaskCreatePinnedToCore()`.

### Task Anatomy

Tasks typically follow this pattern:

```c
void myTask(void *pvParameters) {
    while (1) {
        // Do work
        vTaskDelay(100); // Yield CPU to other tasks
    }
}
```

**Important:** Task functions should never return. They typically run in infinite loops with periodic delays.



## The FreeRTOS Scheduler

The scheduler is best described as a **fixed-priority preemptive scheduler with time slicing**. Here's what that means:

### Fixed Priority

- Each task receives a constant priority upon creation (0 to 24 by default on ESP32)
- Higher numbers = higher priority
- The scheduler always runs the **highest-priority ready task**
- Priority 0 is reserved for the Idle Task

### Preemption

The scheduler can **interrupt a running task** to execute a higher-priority task that becomes ready. This happens automatically without task cooperation.

**Key insight for dual-core ESP32:** Each core independently schedules tasks. When selecting a task, a core chooses the highest-priority ready task that:
- Has compatible affinity (pinned to that core or unpinned)
- Isn't currently running on another core

### Time Slicing (Round-Robin)

When multiple tasks share the same priority, the scheduler switches between them periodically. On ESP32, this is **best-effort round-robin** because tasks may be pinned to specific cores or already running on another core.

### Context Switching

**Context switching** is the process of saving the current task's state (registers, stack pointer, program counter) and loading the next task's state. FreeRTOS handles this automatically during:

- **Preemption:** Higher-priority task becomes ready
- **Time slicing:** Equal-priority tasks share CPU time
- **Blocking:** Task waits for delay or event
- **ISR yield:** Interrupt unblocks a higher-priority task

While context switching is highly optimized, it does consume a few microseconds per switch.



## ESP32 Dual-Core Behavior

The ESP32 has **two identical CPU cores** (Core 0 and Core 1), enabling true parallel task execution. FreeRTOS maintains **separate schedulers** for each core.

### Task Affinity

- **Pinned tasks** run only on their assigned core
- **Unpinned tasks** can migrate between cores for load balancing
- By default, tasks created with `xTaskCreate()` are unpinned

### Core Assignment

When you create an unpinned task, it's initially assigned to the same core as the creator task (usually Core 0 where `app_main` runs). The scheduler may later migrate it to balance load.


## Practical Experiments

Let's explore scheduling behavior through hands-on experiments with three LED tasks.

### Experiment 1: Priority and Preemption

We create two tasks with different priorities and periods:

```c
void toggle_fast(void *pvParameters) {
    while (1) {
        gpio_set_level(LED_FAST, 1);
        ESP_LOGI(TAG, "Task Fast run @ tick %lu on CPU %d", 
                 xTaskGetTickCount(), xPortGetCoreID());
        gpio_set_level(LED_FAST, 0);
        vTaskDelay(100); // 100 ticks = ~100ms
    }
}

// Created with priority 1
xTaskCreate(toggle_fast, "Task_Fast", 2048, NULL, 1, NULL);
```

**Observation:** The task runs every 100 ticks as expected, yielding the CPU during delays.

### Experiment 2: CPU Starvation

Now we add a higher-priority task that monopolizes the CPU:

```c
void task_high(void *pvParameters) {
    while (1) {
        ESP_LOGI(TAG, "High start busy @ tick %lu on CPU %d", 
                 xTaskGetTickCount(), xPortGetCoreID());
        
        TickType_t start_tick = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start_tick) < 500) {
            gpio_set_level(LED_HIGH, 1);
            // Busy loop - NO vTaskDelay()!
        }
        
        gpio_set_level(LED_HIGH, 0);
        vTaskDelay(100); // Brief pause before next cycle
    }
}

// Created with priority 2 (higher than Task_Fast)
xTaskCreate(task_high, "Task_High", 2048, NULL, 2, NULL);
```

**Key Observations:**

1. **First 5 seconds:** `Task_Fast` doesn't run at all on CPU0 because:
   - `Task_High` (priority 2) monopolizes CPU0
   - `Task_Fast` (priority 1) is initially assigned to CPU0
   - Lower priority tasks cannot preempt higher priority tasks

2. **After 5 seconds:** `Task_Fast` migrates to CPU1 and starts running because:
   - The scheduler detects load imbalance
   - CPU1 is mostly idle
   - `Task_Fast` is eligible to migrate (unpinned)

3. **Periodic behavior:** When `Task_High` blocks (during `vTaskDelay`), if CPU0 is free, `Task_Fast` briefly runs there before migrating back to CPU1.

### The Watchdog Timeout Issue

During the CPU monopolization, you might see this error:

```
E (11272) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (11272) task_wdt:  - IDLE0 (CPU 0)
```

**Why this happens:**

- The Task Watchdog monitors Idle tasks on both cores
- Idle tasks must run periodically to perform housekeeping (cleaning up deleted tasks, feeding watchdog)
- When `Task_High` never yields, IDLE0 can't run, triggering the watchdog
- Default timeout is 5 seconds (configurable in `menuconfig → ESP System Settings → Task Watchdog timeout`)

**Solution:** Either increase the timeout or ensure high-priority tasks yield periodically.

### Using `taskYIELD()`

Adding `taskYIELD()` inside the busy loop allows same-priority tasks to run:

```c
while ((xTaskGetTickCount() - start_tick) < 500) {
    gpio_set_level(LED_HIGH, 1);
    taskYIELD(); // Voluntarily yield to same-priority tasks
}
```

**Important:** `taskYIELD()` only yields to tasks of the **same or higher priority**. It won't help lower-priority tasks run.



## Task Control: Suspend and Resume

Sometimes you need to temporarily pause a task without deleting it. Use `vTaskSuspend()` and `vTaskResume()`:

```c
void task_controller(void *pvParameters) {
    while (1) {
        ESP_LOGI(TAG, "Controller suspending Task_Fast");
        vTaskSuspend(task_fast_handle); // Pause the task
        vTaskDelay(200);                // Wait 2 seconds
        
        ESP_LOGI(TAG, "Controller resuming Task_Fast");
        vTaskResume(task_fast_handle);  // Resume the task
        vTaskDelay(200);                // Wait 2 seconds
    }
}
```

### Suspend vs Delay

- **`vTaskDelay()`:** Voluntary block, task enters BLOCKED state, scheduler can pick other tasks
- **`vTaskSuspend()`:** Forced removal from READY list, task won't run until explicitly resumed
- **`vTaskResume()`:** Returns task to READY state, scheduler immediately picks it if it's highest priority

### Practical Uses

- Temporarily pause background tasks without changing priorities
- Implement cooperative CPU sharing or event-driven activation
- Control resource-intensive operations (motors, sensors, communications)

---

## Delays: vTaskDelay vs vTaskDelayUntil

### vTaskDelay()

Suspends the task for a specific number of ticks:

```c
vTaskDelay(100); // Wait ~100ms (assuming 1ms tick)
```

While this task waits, other tasks can run.

### vTaskDelayUntil()

Useful for **periodic tasks with precise timing**:

```c
TickType_t xLastWakeTime = xTaskGetTickCount();
const TickType_t xFrequency = 100;

while (1) {
    // Do work
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
}
```

This maintains fixed intervals regardless of task execution time, perfect for sensor sampling or control loops.



## Stack Management

Each task has its own **private stack** for local variables, return addresses, and function calls.

### Defining Stack Size

Stack size is specified in **words** (1 word = 4 bytes on ESP32):

```c
xTaskCreate(taskFunction, "TaskName", 2048, NULL, 1, NULL);
//                                    ^^^^ 2048 words ≈ 8KB
```

### Monitoring Stack Usage

Check remaining stack space:

```c
UBaseType_t watermark = uxTaskGetStackHighWaterMark(taskHandle);
ESP_LOGI(TAG, "Task Stack Free Space: %u words", watermark);
```

**Best Practice:** Leave at least 100-200 words of free stack as a safety margin.

### Common Stack Problems

1. **Stack Overflow:** Task exceeds allocated stack
   - Enable checking: `configCHECK_FOR_STACK_OVERFLOW 2`
   - Implement `vApplicationStackOverflowHook()`
   - Increase stack size

2. **Watchdog Reset:** Task runs too long without yielding
   - Add `vTaskDelay()` calls
   - Avoid infinite loops without delays

3. **Heap Fragmentation:** Frequent task creation/deletion
   - Use `vTaskSuspend()`/`vTaskResume()` instead
   - Consider static task allocation



## Tick Configuration

The **tick rate** determines scheduler frequency, defined in `FreeRTOSConfig.h`:

```c
#define configTICK_RATE_HZ 1000  // 1ms per tick
```

### Impact on Performance

| Tick Rate | Tick Duration | Use Case | Impact |
|-----------|---------------|----------|--------|
| 1000 Hz | 1 ms | High responsiveness (control loops) | More context switches, higher overhead |
| 100 Hz | 10 ms | General applications | Balanced performance |
| 10 Hz | 100 ms | Low power, slow systems | Lower overhead, slower response |



## Debugging Tools

### ESP-IDF SystemView

Provides real-time visualization of:
- Task execution timeline on both cores
- Context switches and preemption events
- Tick timing and interrupt handling

### Built-in Monitoring Functions

```c
// Check stack usage
uxTaskGetStackHighWaterMark(taskHandle);

// Get CPU time statistics
vTaskGetRunTimeStats(pcWriteBuffer);

// Get current core ID
xPortGetCoreID(); // Returns 0 or 1
```



## Best Practices

1. **Use appropriate priorities:** Avoid too many tasks at the same priority
2. **Always yield in loops:** Use `vTaskDelay()` to prevent CPU monopolization
3. **Monitor stack usage:** Prevent overflow crashes
4. **Pin critical tasks:** Reduce scheduler contention for time-critical operations
5. **Avoid deleting running tasks:** Prefer suspend/resume for task control
6. **Keep tasks short:** Long-running tasks should periodically yield
7. **Use queues for communication:** Safer than shared variables

## References

- [Controllers Tech: Getting Started with FreeRTOS on ESP32](https://controllerstech.com/freertos-esp32-esp-idf-task-management/)
- [Controllers Tech: ESP32 FreeRTOS Scheduler Explained](https://controllerstech.com/esp32-freertos-task-scheduler/)
- [Controllers Tech: Task Priority and Stack Management](https://controllerstech.com/esp32-freertos-task-priority-stack-management/)
- [Espressif: FreeRTOS (IDF) Official Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html)


## Next Steps

Future experiments will explore:
- Inter-task communication with queues and semaphores
- Synchronization with mutexes
- Advanced cross-core behavior and affinity
- ISR integration with task notifications

Happy scheduling! 🚀