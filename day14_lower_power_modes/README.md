First things first, low-power modes are not an optimization, they are a core system requirement in real products.

If you don’t understand low-power modes:

- Your code may function perfectly
- And still make the product **commercially unusable**

A firmware engineer must design behavior **around power**, not just functionality.

Low-power modes teach you that:

- The system is **event-driven**
- The CPU should be **off by default**
- Work happens only when something *forces* it to

When you enter deep sleep on ESP32, RAM may be lost, CPU context is lost, peripherals are reset, only RTC memory / registers may survive

So you must answer:

- What must be saved?
- Where do I store it?
- How do I restore behavior after wake?

This is then naturally continuity to what we previously learned about memory architecure, startup code and system reliability.

If your firmware is inefficient, power measurements will *prove it*.

---

Research notes

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/low-power-mode/low-power-mode-soc.html?utm_source=chatgpt.com

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html

The ESP32 supports various low power modes. From a systemic perspective on power management, the typical modes include DFS, Light-sleep mode, and Deep-sleep mode. 

These modes reduce power consumption by 

- lowering clock frequencies (DFS) or
- entering sleep states without affecting system functionality.

**DFS**

Dynamic Frequency Scaling (DFS)

DFS adjusts the Advanced Peripheral Bus (APB) frequency and CPU frequency based on the application's holding of power locks. When holding a high-performance lock, it utilizes high frequency, while in idle states without holding power locks, it switches to low frequency to reduce power consumption, thereby minimizing the power consumption of running applications as much as possible.

DFS is suitable for scenarios where the CPU must remain active but low power consumption is required. Therefore, DFS is often activated with other low power modes, as will be detailed in the following sections.

**Ligh-sleep Mode**

Users can switch to Light-sleep mode by calling [**`esp_light_sleep_start()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv) interface. Upon entering sleep, the chip will shut down unnecessary power domains and apply clock gating to modules not in use, based on the current operational states of peripherals.
When the chip wakes up from Light-sleep mode, the CPU continues running from the context it was in before entering sleep, and the operational states of peripherals remain unaffected.
To effectively reduce chip power consumption under Light-sleep mode, it is highly recommended that users utilize Auto Light-sleep mode described below.

Auto Light-sleep mode is a low power mode provided by ESP-IDF [Power Management](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/power_management.html) component that leverages FreeRTOS's Tickless IDLE feature. When the application releases all power locks and all FreeRTOS tasks are in a blocked or suspended state, the system automatically calculates the next time point when an event will wake the operating system. If this calculated time point exceeds a set duration ([CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html#config-freertos-idle-time-before-sleep)), the `esp_pm` component automatically configures the timer wake-up source and enters light sleep to reduce power consumption. To enable this mode, users need to set the `light_sleep_enable` field to true in [**`esp_pm_config_t`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/power_management.html#_CPPv415esp_pm_config_t) when configuring DFS. 

```css
                              ┌────────┐
                              │        │
                              │  DFS   │
                              │        │
                              └───┬────┘
                                  │
                                  ▼
┌──────────┐   when idle      ┌──────────┐ exceed set time ┌──────────┐
│          │  ─────────────►  │          │  ────────────►  │          │
│          │                  │          │                 │   auto   │
│  active  │                  │   IDLE   │                 │  light   │
│          │                  │          │                 │   sleep  │
│          │  ◄─────────────  │          │                 │          │
└──────────┘     not idle     └──────────┘                 └──────┬───┘
  ▲                                                               │
  │                      configure wake-up source                 │
  └───────────────────────────────────────────────────────────────┘

                    Auto Light-sleep Mode Workflow
```

**Deep-sleep Mode**

Deep-sleep mode is designed to achieve better power performance by retaining only RTC/LP memory and peripherals during sleep, while all other modules are shut down. 

Similar to Light-sleep mode, Deep-sleep mode is entered through API calls and requires configuration of wake-up sources for awakening. Users can switch to Deep-sleep mode by calling [**`esp_deep_sleep_start()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv420esp_deep_sleep_startv) interface.
Deep-sleep mode requires the configuration of wake-up sources. The ESP32 supports multiple wake-up sources.

If no wake-up source is configured when entering deep sleep, the chip will remain in sleep state until an external reset occurs. 

Unlike Light-sleep mode, Deep-sleep mode upon awakening will lose the CPU's running context before, so the bootloader needs to be run again to enter the user program.

The workflow of Deep-sleep mode is shown as below:

```css
┌───────┐  call API  ┌───────┐
│       ├───────────►│ deep  │
│active │            │ sleep │
│       │            │       │
└───────┘            └───┬───┘
    ▲                    │
    └────────────────────┘
    wake-up source wakes up
  Deep-sleep Mode Workflow
```

The primary application scenario of Deep-sleep mode determines that the system will awaken only after a long period and will return to deep sleep state after completing its task.

Deep-sleep mode can be utilized in low power sensor applications or situations where data transmission is not required for most of the time, commonly referred to as standby mode.

Devices can wake up periodically from deep sleep to measure and upload data, and then return to deep sleep. Alternatively, it can store multiple data sets in RTC memory and transmit them all at once upon the next wake-up. This feature can be implemented using the deep-sleep-stub functionality. 

[Deep-sleep Wake Stubs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/deep-sleep-stub.html)

**Difference between Light and Deep Sleep Modes**

In Light-sleep mode, the digital peripherals, most of the RAM, and CPUs are clock-gated and their supply voltage is reduced. Upon exit from Light-sleep, the digital peripherals, RAM, and CPUs resume operation and their internal states are preserved.

In Deep-sleep mode, the CPUs, most of the RAM, and all digital peripherals that are clocked from APB_CLK are powered off. The only parts of the chip that remain powered on are:

- RTC controller
- ULP coprocessor
- RTC FAST memory
- RTC SLOW memory

### **Wake up sources**

Wakeup sources can be enabled using  `esp_sleep_enable_X_wakeup` APIs. Wakeup sources are not disabled after wakeup, you can disable them using [**`esp_sleep_disable_wakeup_source()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv431esp_sleep_disable_wakeup_source18esp_sleep_source_t) API if you do not need them any more.
**Timer**
The RTC controller has a built-in timer which can be used to wake up the chip after a predefined amount of time. 

**Touchpad**

The RTC IO module contains the logic to trigger wakeup when a touch sensor interrupt occurs. To wakeup from a touch sensor interrupt, users nee

**External Wakeup ext0**

The RTC IO module contains the logic to trigger wakeup when one of RTC GPIOs is set to a predefined logic level. RTC IO is part of the RTC peripherals power domain, so RTC peripherals will be kept powered on during Deep-sleep if this wakeup source is requested.

The RTC IO module is enabled in this mode, so internal pullup or pulldown resistors can also be used. They need to be configured by the application using [**`rtc_gpio_pullup_en()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html#_CPPv418rtc_gpio_pullup_en10gpio_num_t) and [**`rtc_gpio_pulldown_en()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html#_CPPv420rtc_gpio_pulldown_en10gpio_num_t) functions before calling [**`esp_deep_sleep_start()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv420esp_deep_sleep_startv).

**External Wakeup ext1**

The RTC controller contains the logic to trigger wakeup using multiple RTC GPIOs. One of the following two logic functions can be used to trigger ext1 wakeup:

- wake up if any of the selected pins is high (`ESP_EXT1_WAKEUP_ANY_HIGH`)
- wake up if all the selected pins are low (`ESP_EXT1_WAKEUP_ALL_LOW`)

This wakeup source is controlled by the RTC controller. Unlike `ext0`, this wakeup source supports wakeup even when the RTC peripheral is powered down. 

Although the power domain of the RTC peripheral, where RTC IOs are located, is powered down during sleep modes, ESP-IDF will automatically lock the state of the wakeup pin before the system enters sleep modes and unlock upon exiting sleep modes. 

Therefore, the internal pull-up or pull-down resistors can still be configured for the wakeup pin
**ULP Coprocessor Wakeup**

**GPIO Wakeup (light-sleep only)**

**UART Wakeup (light-sleep only)**

### **Power-down option**

The application can force specific powerdown modes for RTC peripherals and RTC memories. In Deep-sleep mode, we can also isolate some IOs to further reduce current consumption.

**Power-down of Flash**

By default, to avoid potential issues, [**`esp_light_sleep_start()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv) function does **not** power down flash. To be more specific, it takes time to power down the flash and during this period the system may be woken up, which then actually powers up the flash before this flash could be powered down completely. As a result, there is a chance that the flash may not work properly.
So, in theory, it is ok if you only wake up the system after the flash is completely powered down. However, in reality, the flash power-down period can be hard to predict
Therefore, it is recommended not to power down flash when using ESP-IDF. 

**Power-down of RTC Peripherals and Memories**

By default,  [**`esp_deep_sleep_start()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv420esp_deep_sleep_startv) and [**`esp_light_sleep_start()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv) functions power down all RTC power domains which are not needed by the enabled wakeup sources. To override this behaviour, [**`esp_sleep_pd_config()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv419esp_sleep_pd_config21esp_sleep_pd_domain_t21esp_sleep_pd_option_t) function is provided.
If some variables in the program are placed into RTC SLOW memory (for example, using `RTC_DATA_ATTR` attribute), RTC SLOW memory will be kept powered on by default. This can be overridden using [**`esp_sleep_pd_config()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv419esp_sleep_pd_config21esp_sleep_pd_domain_t21esp_sleep_pd_option_t) function, if desired.
**configuring IOs (Deep-sleep only)**

Some ESP32 IOs have internal pullups or pulldowns, which are enabled by default. If an external circuit drives this pin in Deep-sleep mode, current consumption may increase due to current flowing through these pullups and pulldowns.

To isolate a pin to prevent extra current draw, call [**`rtc_gpio_isolate()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html#_CPPv416rtc_gpio_isolate10gpio_num_t) function.

**Checking Sleep Wakeup Cause**

[**`esp_sleep_get_wakeup_cause()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv426esp_sleep_get_wakeup_causev) function can be used to check which wakeup source has triggered wakeup from sleep mode.

For touchpad, it is possible to identify which touch pin has caused wakeup using [**`esp_sleep_get_touchpad_wakeup_status()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv436esp_sleep_get_touchpad_wakeup_statusv) functions.

For ext1 wakeup sources, it is possible to identify which GPIO has caused wakeup using [**`esp_sleep_get_ext1_wakeup_status()`**](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv432esp_sleep_get_ext1_wakeup_statusv) functions.

---

We don’t have an oscilloscope or multimeter to measure power consuption so we’ll redirect the focus of the exercice from power consumption to the effect of low power modes on the state of our system.

**Exercice 1 :** 

**Goal :** Prove that deep sleep loses CPU & RAM state

**What you learn**

- Deep sleep ≠ pause
- Bootloader runs again
- `.bss` / `.data` are reinitialized

**What we observed**

First boot:

```css
I (284) BOOT: Counter = 1
I (284) BOOT: Going to deep sleep for 5 seconds...
```

the board resets:

```css
rst:0x5 (DEEPSLEEP_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
```

and second boot:

```css
I (286) BOOT: Counter = 1
I (286) BOOT: Going to deep sleep for 5 seconds...
```

and this repeats undefinitifely.

counter does not become 2, 3, 4, … instead it is always 1 meaning RAM content was lost.

Let’s add an extra line

```css
ESP_LOGI(TAG, "Wakeup cause: %d", esp_sleep_get_wakeup_cause());

```

On the first boot we get this log:

```css
I (284) BOOT: Wakeup cause: 0
```

This changes on the second boot, meaning on waking up from the deep sleep, we get this 

```css
I (286) BOOT: Wakeup cause: 4
```

0 corresponds to *enumerator* ESP_SLEEP_WAKEUP_UNDEFINED, this means that reset was not caused by exit from deep sleep.

4 corresponds to *enumerator* ESP_SLEEP_WAKEUP_TIMER, this means that Wakeup is caused by timer

you find the esp_sleep_source_t enumerator values [here](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html#_CPPv418esp_sleep_source_t).

Let’s see which memory survives deep sleep now.

**Exercice 2**

**Goal:** Demonstrate that RTC memory survives deep sleep

First log:

```css
I (284) BOOT: Wakeup cause: 0
I (284) BOOT: RAM counter = 1
I (284) BOOT: RTC counter    = 1
I (284) BOOT: Entering deep sleep...
```

Second log:

```css
I (286) BOOT: Wakeup cause: 4
I (286) BOOT: RAM counter = 1
I (286) BOOT: RTC counter    = 2
I (286) BOOT: Entering deep sleep...
```

What we understand is

| Variable | Memory | Behavior |
| --- | --- | --- |
| ram_counter | DRAM | Reset every deep sleep |
| rtc_counter | RTC SLOW | Preserved |

However, it is important to note that RTC memory is limited, slower and should be used deliberately. 

We can add a little extra spice to our program with this line

```css
if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
    ESP_LOGI(TAG, "Cold boot detected, resetting RTC counter");
    rtc_counter = 0;
}
```

Now:

- Power cycle → RTC counter resets
- Deep sleep wake → RTC counter increments

```css
I (284) BOOT: Wakeup cause: 0
I (284) BOOT: Cold boot detected, resetting RTC counter
I (284) BOOT: RAM counter = 1
I (284) BOOT: RTC counter    = 1
I (284) BOOT: Entering deep sleep...
-
-
I (286) BOOT: Wakeup cause: 4
I (286) BOOT: RAM counter = 1
I (286) BOOT: RTC counter    = 2
I (286) BOOT: Entering deep sleep...
-
-
I (284) BOOT: Wakeup cause: 0
I (284) BOOT: Cold boot detected, resetting RTC counter
I (284) BOOT: RAM counter = 1
I (284) BOOT: RTC counter    = 1
I (284) BOOT: Entering deep sleep...
```

Now let’s explore other Wakeup Causes

**Exercice 3**

**Goal:** learn to wake via external button, and detect and act on the cause

Hardware set up is simple enough :

- Button connected between GPIO and GND
- use internal pull-up

<aside>
🫠

On page 134 of the esp32 tachnical reference manual, you’ll find a list of GPIO pins that you can use as an RTC IO (table RTC  IO MUX Pin Summary)

</aside>

I’ll use GPIO 4 for this exercice

we will:

- Enable **timer wake-up**
- Enable **EXT0 GPIO wake-up**
- Read `esp_sleep_get_wakeup_cause()`
- Change behavior based on cause

Observations:

Pressing the external button while the board is **not in deep sleep** does not reboot it. 

It is possible to cold reboot (reset button) the board while it is in deepsleep.

Buttons can bounce, which causes multiple wake-ups.

In a future project, we can use these principles, multiple wake-up causes, brancing the frimware behavior based on wake-up cause in our future projects.

| Wake cause | Typical behavior |
| --- | --- |
| Timer | Periodic measurement |
| Button | User interaction |
| Sensor interrupt | Threshold exceeded |
| Communication | Incoming request |

Now that we explored deep sleep, let’s compare it with light sleep

**Exercice 4**

**Goal:** Compare **task/context preservation** in Light Sleep vs Deep Sleep
Observations
when i reset the board, i always get this log

```css
I (275) main_task: Started on CPU0
I (285) main_task: Calling app_main()
I (285) Wake_TES
```

And it stops right there and goes to light sleep without completing the log.
When it wake ups, it the rest of the logs is then sent,  which should've been there berfore the sleep :

```css
T: Wakeup cause: 0
I (285) Wake_TEST: Loop counter: 1
I (285) Wake_TEST: Going to LIGHT sleep for 10s or button press...
I (285) Wake_TEST: Woke up from LIGHT sleep!
I (295) main_task: Returned from app_main()
I (1285) Wake_TEST: Loop counter: 2
I (2285) Wake_TEST: Loop counter: 3
I (3285) Wake_TEST: Loop counter: 4
```

So when I call esp_light_sleep_start(); The UART peripheral stops transmitting temporarily and any logs in the queue that **haven’t actually been sent over UART yet** are **flushed after wake**. 
Then the board enters light sleep **before the rest of the logs are transmitted**.

Later, after wake, UART catches up and prints the remaining messages

I tried fixing it with two solutions:

1. Add a 100ms delay before entering sleep, simple and it fixed it
2. Tried
    
    ```css
    uart_wait_tx_done(CONFIG_ESP_CONSOLE_UART_NUM, pdMS_TO_TICKS(100));
    ```
    
    And it did not work and got me this error
    
    ```css
    I (275) main_task: Started on CPU0
    I (285) main_task: Calling app_main()
    I (285) Wake_TEST: Wakeup cause: 0
    I (285) Wake_TEST: Loop counter: 1
    I (285) Wake_TEST: Going to LIGHT sleep for 10s or button press...
    
    E (285) uart: uart_wait_tx_done(1450): uart driver error
    I (295) Wake_TEST: Woke up from LIGHT sleep!
    ```
    
    Which also was sent after waking up from sleep
    

Logging misshap aside, we observe that the loop resumes counting from where it stoped before entering light sleep

```css
I (285) main_task: Calling app_main()
I (285) Wake_TEST: Wakeup cause: 0
I (285) Wake_TEST: Loop counter: 1
I (285) Wake_TEST: Going to LIGHT sleep for 10s or button press...

E (285) uart: uart_wait_tx_done(1450): uart driver error
I (295) Wake_TEST: Woke up from LIGHT sleep!
I (295) main_task: Returned from app_main()
I (1285) Wake_TEST: Loop counter: 2
I (2285) Wake_TEST: Loop counter: 3
```

If you change this line:

```c
esp_light_sleep_start();
```

to

```c
esp_deep_sleep_start();
```

We observe no logging misshap, and the loop counter resets as it is stored in RAM

```css
I (275) main_task: Started on CPU0
I (285) main_task: Calling app_main()
I (285) Wake_TEST: Wakeup cause: 0
I (285) Wake_TEST: Loop counter: 1
I (285) Wake_TEST: Going to LIGHT sleep for 10s or button press...

ets Jul 29 2019 12:21:46

rst:0x5 (DEEPSLEEP_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
```

so i short

| Sleep Mode | CPU | Tasks | RAM | Context |
| --- | --- | --- | --- | --- |
| Light | Paused | Resumes | Preserved | Everything continues |
| Deep | Off | Reset | Reset | Only RTC memory survives |

now let’s see how lights sleep is implicitly implemented in FreeRTOS

**Exercise 5 — Tickless Idle (Auto Light Sleep)**

**Goal:** understand how the system automatically sleeps when tasks are blocked (no busy-wait loops) →FreeRTOS + ESP-IDF power management in action.

**Configuration**

1. Enable **Tickless Idle** in `sdkconfig`:
    
    ```bash
    idf.py menuconfig
    ```
    
    Navigate to:
    
    ```
    Component config → FreeRTOS → Kernel → Tickless idle support [*] Yes
    ```
    
     If it is still missing (Dependencies) Tickless idle relies on the Power Management component. If that is not enabled, the option might not show up. 
    1. Go to **Component config** →right arrow→ **Power Management**.
    2. Ensure **Support for power management** (`CONFIG_PM_ENABLE`) is checked.
    3. Go back to **Component config** →right arrow→ **FreeRTOS** →right arrow→ **Kernel** and enable tickless idle suppor
    
2. Include the **power management component** in your project:
    
    ```c
    #include"esp_pm.h"
    #include"esp_log.h"
    #include"freertos/FreeRTOS.h"
    #include"freertos/task.h"
    ```
    

**Configure Power Management**

```c
voidconfigure_pm()
{
// Light sleep enabled
esp_pm_config_esp32_t pm_config = {
        .max_cpu_freq =160000000,// 160 MHz max
        .min_cpu_freq =80000000,// 80 MHz min
        .light_sleep_enable =true// Auto light sleep when idle
    };
    esp_pm_configure(&pm_config);
}

```

Call `configure_pm()` **once at the start of `app_main()`**.

Create a blocking task

```c
voididle_task(void *arg)
{
int count =0;
while(1) {
        ESP_LOGI("TASK","Going idle, iteration %d", count++);
// Block for 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

app_main

```c
voidapp_main()
{
    ESP_LOGI("MAIN","Starting Tickless Idle Example");

// Configure power management
    configure_pm();

// Create blocking task
    xTaskCreate(idle_task,"idle_task",2048,NULL,5,NULL);

    ESP_LOGI("MAIN","Tasks created. System will automatically sleep when idle");
}
```

Observations:

Like the previus problem with previous light sleep exemple, uart logs are interrupted by lightsleep

```css
I (301) MAIN: Starting Tickless Idle Example
I (301) pm: Frequency switching config: CPU_MAX: 160, APB_MAX: 80, APB_MIN: 80, Light sleep: ENABLED
I (301) PM: Power management configured successfully
I (311) TASK: Going idle, iterat
```

```css
ion 0
I (311) MAIN: Tasks created. Sys
```

```css
tem will automatically sleep when idle
I
```

```css
(321) main_task: Returned from app_main(
```

```css
)
I (5311) TASK: Going idle, iteration 1
```

this is also a problem for the log inside the task that gets interrupted

 

```css
I (215731) TASK: Going idle, iteration 43
I (2
```

which could be solved with a delay for now. But for i don’t find this solution robust because its effectiveness depends on choosing the rights time period of the delay…

Will look for another solution in the future.