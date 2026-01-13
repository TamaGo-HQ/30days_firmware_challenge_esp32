# Day 11: Critical Sections & ISRs — Real-Time Firmware Fundamentals

## 📌 Overview

Understanding Interrupt Service Routines (ISRs) and critical sections is fundamental to writing reliable firmware. This is where "code that works" diverges from "code that works *reliably*" in production systems.

**Key insight**: The real world is interrupt-driven. Buttons, timers, sensors, and communication peripherals don't wait for your main loop—they demand immediate attention through interrupts.



## 🎯 Learning Objectives

By the end of this day, you should understand:

-  How ISRs safely communicate with RTOS tasks
-  Why critical sections are necessary for data integrity
-  What race conditions look like in embedded systems
-  Why ISRs must be kept minimal
-  How to measure ISR execution time
-  The difference between task-level and ISR-level critical sections



## 🧠 Core Concepts

### 1. **The Interrupt-Driven Reality**

In firmware, important events happen on *their* schedule, not yours:
- Button presses
- UART bytes arriving
- Timer expirations
- ADC conversions completing
- Sensor data ready signals

Missing or mishandling these events leads to nondeterministic behavior—the worst kind of bug.

### 2. **ISRs vs Tasks: Division of Labor**

The golden rule of RTOS design:

> **ISR = Capture event** (fast, minimal)  
> **Task = Process event** (can be slower, complex logic allowed)

**Critical fact**: Even the lowest-priority ISR will preempt the highest-priority task. This is why ISRs must be kept short.

### 3. **Critical Sections: Your Concurrency Shield**

Once you have multiple execution contexts (ISRs, tasks, multiple cores), you have true concurrency. Critical sections protect shared data from race conditions by ensuring atomic execution.

**ESP32 specifics**:
- Dual-core architecture means shared data can be accessed simultaneously
- Use `portENTER_CRITICAL()` / `portEXIT_CRITICAL()` from tasks
- Use `portENTER_CRITICAL_ISR()` / `portEXIT_CRITICAL_ISR()` from ISRs
- Both require a spinlock variable: `portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;`

### 4. **Why Simple Operations Aren't Atomic**

Consider this innocent-looking code:
```c
counter++;
```

This compiles to approximately:
1. Load `counter` from memory to register
2. Increment register
3. Store register back to memory

If an interrupt fires between steps 1 and 3, you can lose counts. This is a **race condition**.


## 🛠️ Exercises Completed

### Exercise 1: ISR → Task Communication

**Goal**: Implement the canonical pattern for ISR-task interaction using FreeRTOS primitives.

**Implementation**:
- GPIO interrupt on falling edge (button press)
- Binary semaphore for signaling
- ISR gives semaphore, task takes it
- Added ISR execution time measurement (2-6 μs typical)

**Key takeaways**:
- ISRs should only signal events, not process them
- Always use `FromISR()` variants of FreeRTOS APIs in interrupt context
- `portYIELD_FROM_ISR()` ensures immediate context switch if higher-priority task was woken
- ISR timing shows small jitter due to cache effects and RTOS overhead—this is normal on modern MCUs

**Interview-ready answer**: "ISR execution time should be *bounded and predictable*, not necessarily constant. Cache misses, RTOS overhead, and multicore effects introduce acceptable jitter on platforms like ESP32."

### Exercise 2: Critical Sections & Race Conditions

**Goal**: Demonstrate why shared variables need protection.

**Implementation**:
- Shared counter incremented by both ISR and task
- Protected using spinlock-based critical sections
- ISR uses `portENTER_CRITICAL_ISR(&spinlock)`
- Task uses `portENTER_CRITICAL(&spinlock)`

**Discovery**: Initial attempt with `NULL` instead of a proper spinlock caused assertion failure:
```
assert failed: spinlock_acquire spinlock.h:84 (lock)
```

**Lesson learned**: ESP32's dual-core architecture requires explicit spinlocks. The `portMUX_TYPE` provides mutual exclusion across both cores.

---

## 💻 Code Structure

```
main.c
├── Spinlock initialization
├── ISR handler (IRAM_ATTR, critical section protected)
├── Event task (blocks on semaphore, critical section protected)
├── GPIO/button initialization
└── app_main (creates semaphore, starts task)
```

**Key attributes used**:
- `IRAM_ATTR`: Places ISR in internal RAM (faster, no cache misses)
- `volatile`: Prevents compiler optimization of shared variables
- `static`: Limits scope appropriately



## 🚨 Common Pitfalls Discovered

| Issue | Symptom | Solution |
|-------|---------|----------|
| NULL spinlock | Assertion failure on critical section | Use `portMUX_INITIALIZER_UNLOCKED` |
| Missing `FromISR` variants | Unpredictable crashes | Always use `xSemaphoreGiveFromISR()`, etc. |
| Long ISR execution | Interrupt latency, missed events | Defer work to tasks |
| Unprotected shared data | Random corruption, lost counts | Use critical sections |



## 📊 ISR Timing Results

Measured execution times for button ISR with semaphore signaling:
- **Best case**: 2-3 μs (rare)
- **Typical case**: 6 μs
- **Variance**: Expected due to cache/RTOS effects

This demonstrates that even minimal ISRs have measurable overhead on real hardware.



## 🎓 Why This Matters

### Real-Time ≠ Fast
Real-time means **bounded, predictable timing**. A "fast but unpredictable" system is worse than a slower, deterministic one.

### This Separates Juniors from Professionals
Many engineers can:
- Write drivers
- Blink LEDs  
- Create tasks

Few can:
- Design safe ISR ↔ task interactions
- Reason about race conditions
- Debug priority inversion and interrupt latency issues
- Explain determinism in their systems

Interviewers probe this area because it reveals systems-level thinking.



## 🔗 Resources Referenced

- [ESP-IDF FreeRTOS Critical Sections Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html#critical-sections)
- [High Integrity Systems RTOS ISR Tutorial](https://www.highintegritysystems.com/rtos/what-is-an-rtos/rtos-tutorials/interrupt-service-routines/)



## 🚀 Next Steps

To deepen understanding:
- [ ] Experiment with different interrupt priorities
- [ ] Test without critical sections to observe race conditions
- [ ] Implement priority inversion scenarios
- [ ] Profile ISR timing with different workloads
- [ ] Study ESP32's interrupt allocation and routing

---

## 💡 Mental Model

> Think of ISRs as emergency responders:  
> They arrive fast, do the absolute minimum, and leave immediately.  
> Everything else belongs in a task.

---

*Part of a 30-day firmware engineering challenge using ESP32 and ESP-IDF.*