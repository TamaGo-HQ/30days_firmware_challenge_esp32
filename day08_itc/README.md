Here’s a clean **README.md** you can directly drop into your project for **Day 08 – Inter-Task Communication (ESP32 / ESP-IDF)**.
It’s structured, concise, and aligned with how you actually experimented today.

---

# Day 08 — Learning Inter-Task Communication (ESP32 / ESP-IDF)

## Overview

This session focused on **Inter-Task Communication (ITC)** mechanisms provided by **FreeRTOS** on the ESP32 using **ESP-IDF**.
The goal was to understand *not just the APIs*, but **their runtime behavior**, scheduling effects, and *when to use each primitive*.

Covered primitives:

* Queues
* Binary Semaphores
* Mutexes (Priority Inheritance)
* Task Notifications

Each section below corresponds to a small experiment toggled in `app_main()`.

---

## Environment

* **Target**: ESP32 (dual-core)
* **Framework**: ESP-IDF
* **OS**: FreeRTOS
* **Host OS**: Windows
* **Logging**: `ESP_LOGI / ESP_LOGW`

---

## 1️⃣ Producer / Consumer with Queue

### Goal

Understand:

* Data ownership
* Blocking behavior
* Queue length vs producer/consumer rate mismatch

### Implementation

* Producer sends an integer counter periodically
* Consumer receives and processes slower
* Queue length intentionally small

```c
QueueHandle_t intQueue;
```

* `xQueueSend()` with timeout
* `xQueueReceive()` with `portMAX_DELAY`

### Key Observations

* **Queues preserve order (FIFO)**
* When full:

  * Producer blocks up to timeout
  * If timeout expires → send fails
* **Data is not overwritten**
* Failed sends mean *that specific value was never queued*

### Concept Reinforced

➡️ **Queues move data AND synchronize tasks**

---

## 2️⃣ Binary Semaphore as Event Signal

### Goal

Learn the difference between:

* Data transfer (queues)
* Event signaling (semaphores)

### Implementation

* Task A periodically gives a binary semaphore
* Task B blocks on `xSemaphoreTake()`
* No data exchanged

```c
SemaphoreHandle_t binarySem;
```

### Key Observations

* Binary semaphore = **event flag**
* Multiple `give()` calls **do not accumulate**
* If given while already available → event is lost
* Task B may preempt immediately when unblocked

### Important Scheduling Insight

> A lower-priority task can run immediately when it is **unblocked by a higher-priority task**, even though it does **not preempt** it in the classical sense.

This is normal FreeRTOS behavior.

### Concept Reinforced

➡️ **Semaphores signal events, not data**

---

## 3️⃣ Priority Inversion: Semaphore vs Mutex

### Goal

Understand **why mutexes exist**.

### Setup

Three tasks pinned to the same core:

| Task   | Priority | Behavior                 |
| ------ | -------- | ------------------------ |
| Low    | 1        | Takes resource, holds it |
| Medium | 2        | CPU hog                  |
| High   | 3        | Tries to take resource   |

### Two Runs

1. Binary Semaphore
2. Mutex

```c
SemaphoreHandle_t resource;
```

### Observations

#### With Binary Semaphore

* High-priority task blocked
* Medium-priority task runs freely
* **Priority inversion occurs**

#### With Mutex

* Low task temporarily inherits high priority
* Medium task is blocked
* High task acquires resource sooner

### Concept Reinforced

➡️ **Mutex = ownership + priority inheritance**
➡️ **Never use semaphores for mutual exclusion**

---

## 4️⃣ Task Notifications vs Queue

### Goal

Understand when **NOT** to use a queue.

### Implementation

* Task A notifies Task B every second
* Task B blocks on notification

```c
xTaskNotifyGive()
ulTaskNotifyTake()
```

### Observations

* Extremely low latency
* Minimal code
* No dynamic allocation
* One notification per task (not many-to-many)

### Comparison

| Feature    | Queue  | Task Notification   |
| ---------- | ------ | ------------------- |
| Memory     | Higher | Very low            |
| Data       | Yes    | Optional (uint32_t) |
| Speed      | Medium | Fastest             |
| Complexity | Higher | Minimal             |

### Concept Reinforced

➡️ **Task notifications are the lightest ITC primitive**

---

## How to Switch Between Experiments

Each experiment is isolated and can be enabled by uncommenting the corresponding block in `app_main()`:

```c
/* ========== Queue ========== */
/* ========== Binary Semaphore ========== */
/* ========== Semaphore vs Mutex ========== */
/* ========== Task Notifications ========== */
```

This makes the file a **single ITC playground**.

---

## Key Learnings Summary

* **Queues**: data + synchronization, FIFO, safe ownership
* **Binary Semaphores**: event signaling only
* **Mutexes**: protect shared resources + prevent priority inversion
* **Task Notifications**: fastest, lightest signaling method
* **Scheduling effects matter more than APIs**

---

## What to Explore Next 🚀

To deepen mastery of FreeRTOS ITC:

1. **Counting Semaphores**

   * Resource pools
   * Limited concurrent access

2. **Queue Sets**

   * Waiting on *multiple* queues/semaphores

3. **ISR-safe APIs**

   * `xQueueSendFromISR`
   * `xSemaphoreGiveFromISR`
   * `xTaskNotifyFromISR`

4. **Timeout tuning**

   * Effects on responsiveness
   * Deadlock scenarios

5. **Core-to-core signaling**

   * Tasks pinned on different cores
   * Cross-core notifications

6. **Tracing & Debugging**

   * `vTaskList()`
   * `uxTaskGetStackHighWaterMark()`
   * FreeRTOS trace tools

---

## Final Note

This day was less about *writing code* and more about **understanding the scheduler’s behavior**.
That’s exactly the right mindset for embedded systems and RTOS-based design.

If you want, next we can:

* Design a **realistic system** using *multiple ITC primitives together*
* Or turn this into a **cheat sheet / decision flowchart** for future projects
