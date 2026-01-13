# Day 2: ESP32 Linker Scripts – Memory and Sections

**Goal:** Understand how `memory.ld` and `sections.ld` organize code and data in the ESP32’s memory.


## 1. Memory regions on the ESP32

The ESP32 has several memory regions where code and data can reside:

| Memory           | Purpose                                                                                         |
| ---------------- | ----------------------------------------------------------------------------------------------- |
| **IRAM**         | Fast, executable RAM. Stores time-critical code (`.iram0.text`) and fast data (`.iram0.data`).  |
| **DRAM**         | General-purpose read/write memory (`.dram0.data`).                                              |
| **RTC FAST**     | RAM that survives deep sleep and can store fast persistent variables (`RTC_DATA_ATTR`).         |
| **RTC SLOW**     | Lower-speed persistent memory, survives deep sleep.                                             |
| **External RAM** | Optional external PSRAM.                                                                        |
| **Flash**        | Non-volatile storage for constants, code (`.text`), and initialized data copied to RAM at boot. |


## 2. The role of linker scripts

ESP32 linker scripts split responsibilities across two main files you can find at %ESP-IDF%/components/esp_system/ld/esp32:

### a) `memory.ld`

* **Defines memory regions**: sets start addresses, lengths, and access types (RX, RW, etc.).
* Example:

```ld
iram0_0_seg (RX) : org = 0x40080000, len = 0x20000 + SRAM1_IRAM_LEN
dram0_0_seg (RW) : org = 0x3FFB0000, len = 0x2C200
rtc_data_seg (RW) : org = 0x3FF80000, len = 0x2000
```

* The author of this file **divides the chip’s address space**, not physical memory itself.
* It also defines symbols like `_heap_start` and `_heap_end`.

### b) `sections.ld`

* **Defines sections**: groups of code/data and their behavior in memory.
* Uses a `SECTIONS{ ... }` block similar to a key-value structure.
* Example for IRAM code:

```ld
.iram0.text :
{
    _iram_text_start = ABSOLUTE(.);
    mapping[iram0_text]
} > iram0_0_seg
```

* Sections map to memory regions defined in `memory.ld`.
* Can specify attributes like `ABSOLUTE`, `aligned_symbol`, `NOLOAD`, etc.

**Key takeaway:**

* `memory.ld` → divides address space
* `sections.ld` → defines what goes into each space and how it behaves


## 3. Sections and attributes

* **Common sections:**

| Section       | Description              |
| ------------- | ------------------------ |
| `.iram0.text` | IRAM-resident code       |
| `.iram0.data` | Fast IRAM variables      |
| `.dram0.data` | General DRAM variables   |
| `.rtc.data`   | Persistent RTC variables |

* **Attributes matter:**

  * `R` → readable
  * `W` → writable
  * `X` → executable
* Example: code cannot be placed in non-X memory; linker enforces this.



## 4. Practical Observation

Example code:

```c
IRAM_ATTR int fast_counter = 0;
RTC_DATA_ATTR int rtc_counter = 0;
DRAM_ATTR int dram_counter = 0;

void app_main(void) {
    fast_counter++; ESP_LOGI("TEST","Fast counter @ %p",&fast_counter);
    rtc_counter++; ESP_LOGI("TEST","RTC counter @ %p",&rtc_counter);
    dram_counter++; ESP_LOGI("TEST","DRAM counter @ %p",&dram_counter);
}
```

**Observed log output:**

```
I (277) TEST: Fast counter: 1
I (277) TEST: Fast counter @ : 0x40083b84
--- 0x40083b84: efuse_hal_get_disable_wafer_version_major at ??:?
I (277) TEST: RTC counter: 1
I (277) TEST: RTC counter @: 0x50000000
I (277) TEST: DRAM counter : 1
I (287) TEST: DRAM counter @: 0x3ffb16ac
```

**Observations:**

* **Fast counter** → `0x40083b84` → IRAM
* **RTC counter** → `0x50000000` → RTC RAM
* **DRAM counter** → `0x3ffb16ac` → DRAM

> The line `--- 0x40083b84: efuse_hal_get_disable_wafer_version_major at ??:?` is **ESP-IDF debug info**. It occurs because ESP-IDF attempts to symbolically resolve addresses in IRAM; if it cannot fully resolve the symbol (or it’s inlined), it prints this placeholder. It does **not indicate an error**.

These addresses can also be cross-checked in the **`day02_linker_scripts.map`** file under the build folder. Relevant lines:

```
.dram1.2       0x3ffb16ac        0x4 esp-idf/main/libmain.a(main.c.obj)
0x3ffb16ac                dram_counter

.rtc.data.1    0x50000000        0x4 esp-idf/main/libmain.a(main.c.obj)
0x50000000                rtc_counter

.iram1.0       0x40083b84        0x4 esp-idf/main/libmain.a(main.c.obj)
0x40083b84                fast_counter
```

**Takeaways:**

* The runtime addresses match **memory regions defined in `memory.ld`**.
* The `.map` file allows verification of **section placement** and **linker behavior**.
* IRAM, DRAM, and RTC sections are correctly separated and mapped as intended.


## 5. Lessons learned

1. `memory.ld` defines **where memory regions exist** in the ESP32 address space.
2. `sections.ld` defines **which code/data goes into which region** and how it behaves.
3. Different memories exist for a reason: execution speed, persistence, and cache behavior.
4. Checking the `.map` file is a **powerful tool** to verify variable placement.


## 6. Exercises / next steps

1. Move a variable between DRAM, IRAM, and RTC → check addresses in logs.
2. Trace a section in `sections.ld` → see which memory region it maps to.
3. Observe how linker attributes (`IRAM_ATTR`, `DRAM_ATTR`, `RTC_DATA_ATTR`) affect placement.

---

**Conclusion:**

You now understand the **purpose of `memory.ld` and `sections.ld`**: controlling how your program’s code and data are distributed across the ESP32 memory. This is foundational for **efficient, safe, and predictable firmware development**.

