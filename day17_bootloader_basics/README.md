In week 1 we briefly explored the topic of our chip’s startup process.

We already covered:

- Boot flow
- Partition table
- OTA slot selection
- `otadata`
- Manual + programmatic switching

Now focus on **what makes this production-safe**.
First, let’s take see what the esp32 documentation says about the bootlaoder

---

The ESP-IDF second stage bootloader performs the following functions:

1. Minimal initial configuration of internal modules;
2. Initialize [Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/flash-encryption.html) and/or [Secure Boot](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/secure-boot-v2.html) features, if configured;
3. Select the application partition to boot, based on the partition table and ota_data (if any);
4. Load this image to RAM (IRAM & DRAM) and transfer management to the image that was just loaded.

ESP-IDF second stage bootloader is located at the address 0x1000 in the flash.

The OTA (over the air) update process can flash new apps in the field but cannot flash a new bootloader.

**SPI Flash Configuration**

Each ESP-IDF application or bootloader .bin file contains a header used to configure the SPI flash during boot.
The [First stage (ROM) bootloader](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/startup.html#first-stage-bootloader) reads the [Second Stage Bootloader](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/startup.html#second-stage-bootloader) header information from flash and uses this information to load the rest of the [Second Stage Bootloader](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/startup.html#second-stage-bootloader) from flash. However, at this time the system clock speed is lower than configured and not all flash modes are supported. When the [Second Stage Bootloader](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/startup.html#second-stage-bootloader) then runs, it will reconfigure the flash using values read from the currently selected app binary's header (and NOT from the [Second Stage Bootloader](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/startup.html#second-stage-bootloader) header). This allows an OTA update to change the SPI flash settings in use.
**Log Level**
The default bootloader log level is "Info". Reducing bootloader log verbosity can improve the overall project boot time by a small amount.
**Factory Reset**

Sometimes it is desirable to have a way for the device to fall back to a known-good state, in case of some problem with an update.

The factory reset mechanism allows the device to be factory reset in two ways:

- Clear one or more data partitions. The [CONFIG_BOOTLOADER_DATA_FACTORY_RESET](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-data-factory-reset) option allows users to specify which data partitions will be erased when the factory reset is executed.
    
    Users can specify the names of partitions as a comma-delimited list with optional spaces for readability. (Like this: `nvs, phy_init, nvs_custom`).
    
    Make sure that the names of partitions specified in the option are the same as those found in the partition table. Partitions of type "app" cannot be specified here.
    
- Boot from "factory" app partition. Enabling the [CONFIG_BOOTLOADER_OTA_DATA_ERASE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-ota-data-erase) option will cause the device to boot from the default "factory" app partition after a factory reset (or if there is no factory app partition in the partition table then the default ota app partition is selected instead). This reset process involves erasing the OTA data partition which holds the currently selected OTA partition slot. The "factory" app partition slot (if it exists) is never updated via OTA, so resetting to this allows reverting to a "known good" firmware application.

Either or both of these configuration options can be enabled independently.

In addition, the following configuration options control the reset condition:

- [CONFIG_BOOTLOADER_NUM_PIN_FACTORY_RESET](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-num-pin-factory-reset)The input GPIO number used to trigger a factory reset. This GPIO must be pulled low or high (configurable) on reset to trigger this.
- [CONFIG_BOOTLOADER_HOLD_TIME_GPIO](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-hold-time-gpio)this is hold time of GPIO for reset/test mode (by default 5 seconds). The GPIO must be held continuously for this period of time after reset before a factory reset or test partition boot (as applicable) is performed.
- [CONFIG_BOOTLOADER_FACTORY_RESET_PIN_LEVEL](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-factory-reset-pin-level) - configure whether a factory reset should trigger on a high or low level of the GPIO. If the GPIO has an internal pullup then this is enabled before the pin is sampled, consult the ESP32 datasheet for details on pin internal pullups.

If an application needs to know if the factory reset has occurred, users can call the function **`bootloader_common_get_rtc_retain_mem_factory_reset_state()`**.
Note that this feature reserves some RTC FAST memory
**Boot from test Firmware**

It is possible to write a special firmware app for testing in production, and boot this firmware when needed. The project partition table will need a dedicated app partition entry for this testing app, type `app` and subtype `test`

Implementing a dedicated test app firmware requires creating a totally separate ESP-IDF project for the test app (each project in ESP-IDF only builds one app). The test app can be developed and tested independently of the main project, and then integrated at production testing time as a pre-compiled .bin file which is flashed to the address of the main project's test app partition.

To support this functionality in the main project's bootloader, set the configuration item [CONFIG_BOOTLOADER_APP_TEST](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-app-test).

**Watchdog**

The chips come equipped with two groups of watchdog timers: Main System Watchdog Timer (MWDT_WDT) and RTC Watchdog Timer (RTC_WDT). Both watchdog timer groups are enabled when the chip is powered up. However, in the bootloader, they will both be disabled. If [CONFIG_BOOTLOADER_WDT_ENABLE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-wdt-enable) is set (which is the default behavior), RTC_WDT is re-enabled. It tracks the time from the bootloader is enabled until the user's main function is called. In this scenario, RTC_WDT remains operational and will automatically reset the chip if no application successfully starts within 9 seconds. This functionality is particularly useful in preventing lockups caused by an unstable power source during startup.
**Bootloader Size**

When enabling additional bootloader functions, including [Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/flash-encryption.html) or Secure Boot, and especially if setting a high [CONFIG_BOOTLOADER_LOG_LEVEL](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-log-level) level, then it is important to monitor the bootloader .bin file's size.
If the bootloader binary is too large, then the bootloader build will fail with an error "Bootloader binary size [..] is too large for partition table offset". If the bootloader binary is flashed anyhow then the ESP32 will fail to boot - errors will be logged about either invalid partition table or invalid bootloader checksum.

Options to work around this are:

- Set [bootloader compiler optimization](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-compiler-optimization) back to "Size" if it has been changed from this default value.
- Reduce [bootloader log level](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-log-level). Setting log level to Warning, Error or None all significantly reduce the final binary size (but may make it harder to debug).
- Set [CONFIG_PARTITION_TABLE_OFFSET](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-partition-table-offset) to a higher value than 0x8000, to place the partition table later in the flash. This increases the space available for the bootloader. If the [partition table](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/partition-tables.html) CSV file contains explicit partition offsets, they will need changing so no partition has an offset lower than `CONFIG_PARTITION_TABLE_OFFSET + 0x1000`. (This includes the default partition CSV files supplied with ESP-IDF.)

**Fast Boot from Deep-Sleep**

The bootloader has the  [CONFIG_BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-skip-validate-in-deep-sleep) option which allows the wake-up time from Deep-sleep to be reduced (useful for reducing power consumption). This option is available when the [CONFIG_SECURE_BOOT](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-secure-boot) option is disabled or [CONFIG_SECURE_BOOT_INSECURE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-secure-boot-insecure) is enabled along with Secure Boot. The reduction in time is achieved by ignoring image verification.
During the first boot, the bootloader stores the address of the application being launched in the RTC FAST memory. After waking up from deep sleep, this address is used to boot the application again without any checks, resulting in a significantly faster load.

---

### Partition tables

A single ESP32's flash can contain multiple apps, as well as many different kinds of data (calibration data, filesystems, parameter storage, etc). For this reason a partition table is flashed to ([default offset](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig-reference.html#config-partition-table-offset)) 0x8000 in the flash.
The partition table length is 0xC00 bytes, as we allow a maximum of 95 entries. Each entry in the partition table has a name (label), type (app, data, or something else), subtype and the offset in flash where the partition is loaded.

The simplest way to use the partition table is to open the project configuration menu (`idf.py menuconfig`) and choose one of the simple predefined partition tables under [CONFIG_PARTITION_TABLE_TYPE](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig-reference.html#config-partition-table-type):

- "Single factory app, no OTA"
- "Factory app, two OTA definitions"

In both cases the factory app is flashed at offset 0x10000. If you execute `idf.py partition-table` then it will print a summary of the partition table.

**Built in Partition tables**

Here is the summary printed for the "Single factory app, no OTA" configuration:

```css
# ESP-IDF Partition Table
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 1M,
```

- At a 0x10000 (64 KB) offset in the flash is the app labelled "factory". The bootloader runs this app by default.
- There are also two data regions defined in the partition table for storing NVS library partition and PHY init data.

Here is the summary printed for the "Factory app, two OTA definitions" configuration:

```css
# ESP-IDF Partition Table
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000,  1M,
ota_0,    app,  ota_0,   0x110000, 1M,
ota_1,    app,  ota_1,   0x210000, 1M,
```

- There are now three app partition definitions. The type of the factory app (at 0x10000) and the next two "OTA" apps are all set to "app", but their subtypes are different.
- There is also a new "otadata" slot, which holds the data for OTA updates. The bootloader consults this data in order to know which app to execute. If "ota data" is empty, it will execute the factory app.

---

### OTA Process Overview

The OTA update mechanism allows a device to update itself based on data received while the normal firmware is running (for example, over Wi-Fi, Bluetooth or Ethernet).

The following modes support OTA updates for certain partitions:

**Safe update mode**. The update process for certain partitions is designed to be resilient, ensuring that even if the power is cut off during the update, the chip will remain operational and capable of booting the current application. 

The following partitions support this mode:

- Application. The OTA operation functions write a new app firmware image to whichever OTA app slot that is currently not selected for booting. Once the image is verified, the OTA Data partition is updated to specify that this image should be used for the next boot.

**Unsafe update mode**. The update process is vulnerable, meaning that a power interruption during the update can cause issues that prevent the current application from loading, potentially leading to an unrecoverable state.

The following partitions support this mode:

- Bootloader.
- Partition table.
- Other data partitions like NVS, FAT, etc.

**OTA Data partition**

An OTA data partition (type `data`, subtype `ota`) must be included in the [Partition Tables](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/partition-tables.html) of any project which uses the OTA functions.

For factory boot settings, the OTA data partition should contain no data (all bytes erased to 0xFF). In this case, the ESP-IDF second stage bootloader boots the factory app if it is present in the partition table. If no factory app is included in the partition table, the first available OTA slot (usually `ota_0`) is booted.

After the first OTA update, the OTA data partition is updated to specify which OTA app slot partition should be booted next.

**App Rollback**

The main purpose of the application rollback is to keep the device working after the update. This feature allows you to roll back to the previous working application in case a new application has critical errors. When the rollback process is enabled and an OTA update provides a new version of the app, one of three things can happen:

- The application works fine, [**`esp_ota_mark_app_valid_cancel_rollback()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html?utm_source=chatgpt.com#_CPPv438esp_ota_mark_app_valid_cancel_rollbackv)  marks the running application with the state `ESP_OTA_IMG_VALID`. There are no restrictions on booting this application.
- The application has critical errors and further work is not possible, a rollback to the previous application is required, [**`esp_ota_mark_app_invalid_rollback_and_reboot()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html?utm_source=chatgpt.com#_CPPv444esp_ota_mark_app_invalid_rollback_and_rebootv) marks the running application with the state `ESP_OTA_IMG_INVALID` and reset. This application will not be selected by the bootloader for boot and will boot the previously working application.
- If the  [CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-app-rollback-enable) option is set, and a reset occurs without calling either function then the application is rolled back.

**App OTA State**

| **States** | **Restriction of selecting a boot app in bootloader** |
| --- | --- |
| ESP_OTA_IMG_VALID | None restriction. Will be selected. |
| ESP_OTA_IMG_UNDEFINED | None restriction. Will be selected. |
| ESP_OTA_IMG_INVALID | Will not be selected. |
| ESP_OTA_IMG_ABORTED | Will not be selected. |
| ESP_OTA_IMG_NEW | If [CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-app-rollback-enable) option is set it will be selected only once. In bootloader the state immediately changes to `ESP_OTA_IMG_PENDING_VERIFY`. |
| ESP_OTA_IMG_PENDING_VERIFY | If [CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-bootloader-app-rollback-enable) option is set it will not be selected, and the state will change to `ESP_OTA_IMG_ABORTED`. |

**Unexpected Reset**

If a power loss or an unexpected crash occurs at the time of the first boot of a new application, it will roll back the application.

Recommendation: Perform the self-test procedure as quickly as possible, to prevent rollback due to power loss.

Only `OTA` partitions can be rolled back. Factory partition is not rolled back.

**Booting Invalid/aborted Apps**

Booting an application which was previously set to `ESP_OTA_IMG_INVALID` or `ESP_OTA_IMG_ABORTED` is possible:

- Get the last invalid application partition [**`esp_ota_get_last_invalid_partition()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html?utm_source=chatgpt.com#_CPPv434esp_ota_get_last_invalid_partitionv).
- Pass the received partition to [**`esp_ota_set_boot_partition()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html?utm_source=chatgpt.com#_CPPv426esp_ota_set_boot_partitionPK15esp_partition_t), this will update the `otadata`.
- Restart [**`esp_restart()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html#_CPPv411esp_restartv). The bootloader will boot the specified application.

To determine if self-tests should be run during startup of an application, call the [**`esp_ota_get_state_partition()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html?utm_source=chatgpt.com#_CPPv427esp_ota_get_state_partitionPK15esp_partition_tP20esp_ota_img_states_t) function. If result is `ESP_OTA_IMG_PENDING_VERIFY` then self-testing and subsequent confirmation of operability is required.

---

Distilled

**The bootloader**

- configures the flash using values set in th app binary.
- can execute a factory reset in wich data partitions
- can be erased, partitions of type app cannot be erased here or/and a factory reset can make the device boot from the default "factory" app partition after a factory reset.
- the factory app partition is never updated via ota
- can boot from a test firmware, from a partition of type app and subtype test

The RTC_WDT tracks the time from the bootloader is enabled until user's main function is called. The chip is reset if no application successfully starts within 9 seconds

There s an option for a fast boot from deep sleep which reduces wake up time from deep-sleep

There s two OTA update modes

- safe update mode for application partitions
- unsafe update mode for bootloader partition table and other data partitions like NVS, FAT ect...
The difference is that a power interruption during an unsafe update can cause issues that prevent the current application from loading, potentially leading to an unrecoverable state. For the safe mode however, even if the power is cut off during the update, the chip will remain operational and capable of booting the current application.

| Mode | Partitions | Safe if power loss? | Notes |
| --- | --- | --- | --- |
| **Safe** | Application | yes | OTA ensures device can still boot current app |
| **Unsafe** | Bootloader, partition table, NVS, FAT | no | Power loss may brick app |

**ESP32 OTA State Machine (Rollback Enabled)**

```css
              ┌──────────────────────────────┐
              │      OTA Update              │
              │  (esp_ota_set_boot_partition)│
              └─────────────┬────────────────┘
                            │
                            │  otadata written
                            │  (bootloader-owned)
                            ▼
                 ┌─────────────────────┐
                 │  ESP_OTA_IMG_NEW    │
                 │  (never booted)     │
                 └─────────┬───────────┘
                           │
            Bootloader boots image
           (first boot after OTA)
                           │
                           ▼
         ┌─────────────────────────────────┐
         │ ESP_OTA_IMG_PENDING_VERIFY      │
         │ (bootloader marks automatically)│
         └───────────┬───────────┬─────────┘
                     │           │
     App confirms OK │           │ App crashes / resets
 (esp_ota_mark_app_  │           │ before confirmation
 valid_cancel_rollback)          │
                     │           │
         otadata updated         │
        (by application)         │
                     │           │
                     ▼           ▼
        ┌─────────────────┐   ┌─────────────────┐
        │ ESP_OTA_IMG_    │   │ ESP_OTA_IMG_    │
        │ VALID           │   │ ABORTED         │
        │ (normal boot    │   │ (bootloader)    │
        └─────────────────┘   └─────────┬───────┘
                                        │
                               Bootloader rollback
                               (automatic)
                                        │
                                        ▼
                         ┌─────────────────────────┐
                         │ Previous VALID firmware │
                         │ (factory or older OTA)  │
                         └─────────────────────────┘

```

**Who changes the state?**

Bootloader (automatic, no app control)

- `NEW → PENDING_VERIFY`
- `PENDING_VERIFY → ABORTED`
- Performs **rollback**
- Selects partition at boot
- Writes **otadata**

Application (explicit, voluntary)

- PENDING_VERIFY → VALID
    
    Only happens when the app calls
    
    ```css
    esp_ota_mark_app_valid_cancel_rollback();
    ```
    

**When does rollback happen automatically?**

- Image is PENDING_VERIFY and a reset occurs before confirmation

Reset can be crash, watchdog, power loss, manual reset..

Then the state will move from PENDING_VERIFY → ABORTED → rollback

**Why otadata exists**

- Bootloader **cannot rely on NVS**: not initialized, needs heap, wear-leveling, unsafe during early boot.
- `otadata` = **small, fixed, crash-safe, bootloader-owned structure**
- Ensures **safe OTA, rollback, deterministic boot**.

**Debugging OTA Failure**

![image.png](attachment:33f61050-e47e-43be-a0cf-63f038c461f3:image.png)

We’ll wrap it up with these research notes today and dedicate a future session for practical exercices on the esp32

References

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/startup.html

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/bootloader.html?utm_source=chatgpt.com

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html?utm_source=chatgpt.com

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html?utm_source=chatgpt.com