
# ESP32 Memory & Execution Experiments

This project is a **hands-on exploration of the ESP32 memory architecture**, inspired by the *System and Memory* chapter of the ESP32 Technical Reference Manual.

Rather than reading theory alone, this application lets you **observe memory behavior directly** by running small, focused experiments on a real ESP32.

The goal is to answer questions like:

* Where does my code actually run from?
* What survives deep sleep, and what doesn’t?
* Why does cache exist?
* What’s the real difference between Flash, IRAM, DRAM, and RTC memory?


## How to Use This Project

All experiments live in the same `main.c`.

The `app_main()` function is intentionally structured so that **only one experiment runs at a time**.
To try an experiment:

1. Open `main.c`
2. Uncomment **one section** in `app_main()`
3. Flash and run
4. Observe output and behavior
5. Comment it back and move to the next one

All projects are built in an ESP-IDF environment


## Background Concepts (Short Theory)

### ESP32 Architecture

* Dual-core Xtensa LX6:

  * **PRO_CPU**
  * **APP_CPU**
* Harvard architecture:

  * Separate **instruction bus** and **data bus**
* Address mapping is **symmetric**:

  * Both CPUs use the same addresses to access the same memory

Address ranges (simplified):

* `< 0x4000_0000` → Data bus
* `0x4000_0000 – 0x4FFF_FFFF` → Instruction bus
* `≥ 0x5000_0000` → Shared / peripheral / RTC regions

---

### Embedded Memory Overview

| Memory        | Size   | Purpose                             |
| ------------- | ------ | ----------------------------------- |
| ROM           | 448 KB | Bootloader & low-level system code  |
| Internal SRAM | 520 KB | IRAM + DRAM                         |
| RTC FAST      | 8 KB   | Fast retained memory (PRO_CPU only) |
| RTC SLOW      | 8 KB   | Low-power retained memory           |

---

### Flash, Cache, and XIP

* Program code lives in **external SPI Flash**
* Flash is **slow**
* ESP32 uses:

  * **MMU** → maps flash into address space
  * **Transparent cache** → hides flash latency

This allows **Execute In Place (XIP)**:

> The CPU *appears* to execute code from memory, but instructions are actually fetched from Flash into cache automatically.

⚠️ If cache is disabled:

* Flash code cannot execute
* Only **IRAM code** can run

---

### IRAM vs DRAM

* **IRAM**

  * Holds instructions
  * Executed directly by CPU
  * No cache, no flash latency
  * Required for interrupts and cache-disabled periods

* **DRAM**

  * Holds data
  * Stack, heap, globals
  * Accessible by CPU **and DMA**

---

### RTC Memory

* Retained during **deep sleep**
* Two types:

  * **RTC FAST** → faster, PRO_CPU only
  * **RTC SLOW** → low-power storage

Used for:

* Counters
* State variables
* Wake-up context

---

### DMA Memory

DMA (Direct Memory Access):

* Moves data **without CPU involvement**
* Used by I2S, SPI, UART, ADC, etc.

Constraints:

* DMA can only access **specific memory regions**
* Typically requires:

  * Internal DRAM
  * `MALLOC_CAP_DMA`

---

### PID (Peripheral ID) Controller

This is **not a control PID**.

PID = **Peripheral ID**

Its role:

* Identify which CPU (PRO or APP) initiated a transaction
* Route memory and peripheral accesses correctly
* Prevent illegal cross-core access

This matters because ESP32 is **dual-core**.


## Experiments in This Project

### 1️⃣ RTC vs DRAM (Deep Sleep Retention)

**What you do**

* Blink an LED
* Increment two variables:

  * `normal_var` (DRAM)
  * `rtc_var` (RTC slow memory)
* Enter deep sleep for 5 seconds
* Wake up and print values

**Key observation**

* `normal_var` resets after deep sleep
* `rtc_var` keeps incrementing

**Addresses observed**

* `normal_var`: `0x3ffb28f4` (DRAM)
* `rtc_var`: `0x50000000` (RTC region)
*  `blink_led_task` with IRAM_ATTR: `0x40082594` (Internal SRAM)
* `blink_led_task` without IRAM_ATTR: `0x400d60c0` (Cache Region)

**Lesson**

> RTC memory is the correct place for persistent state across deep sleep.

---

### 2️⃣ IRAM vs Flash Execution

Two tight loops:

* One placed in Flash (default)
* One placed in IRAM using `IRAM_ATTR`

Measured execution time:

```
Flash loop time: 125098 us
IRAM loop time:  125146 us
```

**Why times are similar**

* For small loops, the difference is tiny; with larger loops or more complex functions, the time gap would be more noticeable.

**Important distinction**

* IRAM guarantees execution **even when cache is disabled**
* Flash code does not

---

### 3️⃣ DMA vs Normal DRAM (I2S Transfer)

Buffers:

* DMA-capable buffer (`MALLOC_CAP_DMA`)
* Normal DRAM buffer

I2S write timings:

```
DMA buffer:     10331 us
Normal DRAM:    11405 us
```

**Lesson**

* DMA-capable memory allows peripherals to transfer data directly
* CPU involvement is reduced
* Performance difference increases with larger transfers



## Why This Project Matters

This project bridges the gap between:

* Reading a reference manual
* Writing real firmware

By running these experiments, you learn:

* Why cache exists
* Why IRAM exists
* Why DMA has restrictions
* Why RTC memory is special
* How address ranges reflect physical memory

These are **core firmware concepts** that apply far beyond ESP32.




## Final Note

This is not a “demo app”.

It’s a **firmware playground** — meant to be modified, broken, measured, and understood.


