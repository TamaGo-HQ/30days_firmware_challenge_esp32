# Day 03: ESP32 Startup & Boot Process

## Overview

Welcome to Day 03 of the firmware learning challenge! After understanding memory architecture (Day 01) and linker scripts (Day 02), we now explore the **startup code** - the missing glue that brings everything together.

**The relationship:**
- **Linker scripts** define *where* sections live in memory
- **Startup code** defines *what to do* with those sections

By the end of this exercise, you'll understand the complete boot sequence from power-on to `app_main()`, and you'll manipulate partition tables to switch between different firmware images.

---

## The Complete Boot Sequence

```
[RESET]
   ↓
ROM Bootloader (1st stage)
   ↓
SPI Bootloader (2nd stage)
   ↓
start_cpu0() → ESP-IDF Entry Point
   ↓
FreeRTOS Startup
   ├─ Create main_task
   ├─ Start scheduler
   ↓
main_task
   └─ app_main()  ← Your code starts here!
```


## Stage 1: ROM Bootloader (1st Stage)

**Location:** Mask ROM (cannot be modified)

**What happens:**
After SoC reset, PRO CPU will start running immediately, executing reset vector code, while APP CPU will be held in reset.

Reset vector code is located in the mask ROM of the ESP32 chip and cannot be modified.

it initializes basic hardware (clocks and minimal peripherals required for boot). Checks strapping pins (whose values are ***sampled by the ROM Bootloader immediately after reset).*** Selects boot mode : either dowload mode (for flashing) or run mode. Loads 2nd stage bootloader if in run mode. Performs intitial verification of 2nd stage bootloader if Secure Boot is enabled

**Reset triggers:**
- Power-on reset
- Deep sleep wake-up
- Software CPU/SoC reset
- Watchdog CPU/SoC reset

---

## Stage 2: SPI Bootloader (2nd Stage)

**Location:** Flash at offset `0x1000`  
**Source code:** `components/bootloader` in ESP-IDF

**What it does:**

Second stage bootloader reads the partition table found by default at offset 0x8000 . the partition table, also flashed into the device,  ****defines named regions (partitions) within the Flash, specifying their type, subtype, offset, and size. note that the 2nd stage bootloader partition is not typically listed in the partition table itself as its location is fixed

- For segments with load addresses in internal [IRAM](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/memory-types.html#iram) or [DRAM](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/memory-types.html#dram), the contents are copied from flash to the load address.
- For segments which have load addresses in [DROM](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/memory-types.html#drom)  (Data Stored in flash) or [IROM](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/memory-types.html#irom) (Code Executed from flash) regions, the flash MMU is configured to provide the correct mapping from the flash to the load address.
 

Once all segments are processed - meaning code is loaded and flash MMU is set up, second stage bootloader verifies the integrity of the application and then jumps to the application entry point found in the binary image header.

> **Memory types:**
> - **IRAM**: Instruction RAM (code executed from RAM)
> - **DRAM**: Data RAM
> - **IROM**: Code executed directly from Flash (via cache)
> - **DROM**: Data stored in Flash (read-only via cache)


## Stage 3: Application Startup (ESP-IDF)

This covers everything that happens after the app starts executing and before the `app_main` function starts running inside the main task. This is split into three stages:

- Port initialization of hardware and basic C runtime environment.
- System initialization of software services and FreeRTOS.
- Running the main task and calling `app_main`.

## Working with Partition Tables

### Default Partition Table

I’ve run idf.py -p COM9 partition-table with day02's project and got:

```
Name       Type    SubType  Offset   Size    Flags
nvs        data    nvs      0x9000   24K
phy_init   data    phy      0xf000   4K
factory    app     factory  0x10000  1M
```

### Custom Partition Table

To experiment with OTA partitions, I created a new project and a custom `partitions.csv`:

```csv
# Name      Type    SubType  Offset   Size
nvs,        data,   nvs,     0x9000,  24K
phy_init,   data,   phy,     0xf000,  4K
ota_data,   data,   ota,     0x10000, 8K
ota_0,      app,    ota_0,   0x20000, 1M
ota_1,      app,    ota_1,   0x120000,1M
```

**Configuration steps:**
1. Set Flash size to 4MB in `menuconfig` (Serial Flasher Config) as 2MB Flash will cause an overflow
2. Enable custom partition table in `menuconfig` (Partition Table → Custom)
3. Specify `partitions.csv` as the filename

The `sdkconfig` will reflect:
```makefile
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_OFFSET=0x8000
```



## Partition Switching Demo

### The Experiment

This demo shows how the bootloader decides which partition to run, **without performing actual OTA updates**. It's purely for learning about partition management.

### Build and Flash

**1. Build the OTA_0 app:**
```c
void app_main(void) {
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    
    ESP_LOGI("OTA", "From OTA_0 Switching to partition: %s", next->label);
    esp_ota_set_boot_partition(next);
    
    ESP_LOGI("OTA", "Rebooting...");
    esp_restart();
}
```

```bash
idf.py build
esptool.py -p COM9 write_flash 0x20000 build/day03_startup.bin
```

**2. Change the log message and flash to OTA_1:**
```c
ESP_LOGI("OTA", "From OTA_1 Switching to partition: %s", next->label);
```

```bash
idf.py build
esptool.py -p COM9 write_flash 0x120000 build/day03_startup.bin
```

**3. Monitor the seesaw between the partitions:**
```bash
idf.py monitor
```

You'll see the device continuously switch between OTA_0 and OTA_1! 🎉

### Boot logs:
```
I (175) boot: Loaded app from partition at offset 0x20000
...
Hello from OTA_0 SLOT
I (xxx) OTA: From OTA_0 Switching to partition: ota_1
I (xxx) OTA: Rebooting...
```

---

### Understanding the OTA Data Partition

The `ota_data` partition (offset `0x10000`, size 8KB) tells the bootloader which OTA partition to boot.

**Inspect it:**
```bash
esptool.py -p COM9 read_flash 0x10000 0x2000 otadata.bin
```

View `otadata.bin` in a hex editor ([HexEd.it](https://hexed.it/)):
- Starts with `0x01` → Boot from `ota_0`
- Starts with `0x02` → Boot from `ota_1`

## Security Features (Brief Overview)

⚠️ **Warning:** These features burn eFuses (one-time programmable). Incorrect configuration can permanently brick your device!

- **Secure Boot:** Ensures only authentically signed firmware can execute
- **Flash Encryption:** Encrypts all Flash contents (bootloader, apps, data)

Always test thoroughly on development kits before production.


## Key Takeaways

1. **Boot flow:** ROM → SPI Bootloader → ESP-IDF startup → FreeRTOS → `app_main()`
2. **Startup code** initializes hardware, runtime, and system services before your code runs
3. **Partition tables** are flexible and can be customized for different use cases
4. **OTA data partition** controls which app partition boots
5. The bootloader, partition table, and application are all **separate entities in Flash**


## References

- [ESP-IDF Application Startup Flow](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/startup.html)
- [Partition Tables Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/partition-tables.html)
- [Understanding ESP32 Boot Process](https://circuitlabs.net/understanding-esp32-boot-process/)
- [ESP32 Boot Mode Selection](https://github.com/espressif/esptool/wiki/ESP32-Boot-Mode-Selection)


## Next Steps

- [ ] Research strapping pins and how ROM bootloader selects boot modes
- [ ] Explore actual OTA update mechanisms
- [ ] Experiment with Secure Boot and Flash Encryption (on a spare dev board!)

---

**Previous:** [Day 02 - Linker Scripts]()  
**Next:** [Day 04 - ?]()