[Watchdog Timer - Introduction and Importance | Microcontroller Basics](https://www.youtube.com/watch?v=7Xcy3k0hj8Q)

[Watchdogs - ESP32 - — ESP-IDF Programming Guide v5.0-beta1 documentation](https://docs.espressif.com/projects/esp-idf/en/v5.0-beta1/esp32/api-reference/system/wdts.html?utm_source=chatgpt.com)

ESP IDF has support for two types of watchdog timers:

- Interrupt Watchdog Timer (IWDT)
- Task Watchodog Timer (TWDT)

These can be enabled in the Project Configuration Menu. TWDT can also be enabled during runtime

### **IWDT**

The purpose of the IWDT is to ensure that interrupt service routines (ISRs) are not blocked from running for a prolonged period of time (i.e., the IWDT timeout period).

The things that can block ISRs from running include:

- Disabling interrupts
- Critical Sections (also disables interrupts)
- Other same/higher priority ISRs (will block same/lower priority ISRs from running it completes execution)

The IWDT utilizes the watchdog timer in Timer Group 1 as its underlying hardware timer and leverages the FreeRTOS tick interrupt on each CPU to feed the watchdog timer.

When the IWDT times out, the default action is to invoke the panic handler and display the panic reason as `Interrupt wdt timeout on CPU0` or `Interrupt wdt timeout on CPU1` (as applicable). Depending on the panic handler’s configured behavior (see [CONFIG_ESP_SYSTEM_PANIC](https://docs.espressif.com/projects/esp-idf/en/v5.0-beta1/esp32/api-reference/kconfig.html#config-esp-system-panic)), users can then debug the source of the IWDT timeout (via the backtrace, OpenOCD, gdbstub etc) or simply reset the chip (which may be preferred in a production environment).
If for whatever reason the panic handler is unable to run after an IWDT timeout, the IWDT has a secondary timeout that will hard-reset the chip (i.e., a system reset).

**Tuning**

If you find the IWDT timeout is triggered because an interrupt or critical section is running longer than the timeout period, consider rewriting the code:

- Critical sections should be made as short as possible. Any non-critical code/computation should be placed outside the critical section.
- Interrupt handlers should also perform the minimum possible amount of computation. Users can consider deferring any computation to a task by having the ISR push data to a task using queues.

Neither critical sections or interrupt handlers should ever block waiting for another event to occur. If changing the code to reduce the processing time is not possible or desirable, it’s possible to increase the [CONFIG_ESP_INT_WDT_TIMEOUT_MS](https://docs.espressif.com/projects/esp-idf/en/v5.0-beta1/esp32/api-reference/kconfig.html#config-esp-int-wdt-timeout-ms) setting instead.

### TWDT

The Task Watchdog Timer (TWDT) is used to monitor particular tasks, ensuring that they are able to execute within a given timeout period. The TWDT primarily watches the Idle Tasks of each CPU, however any task can subscribe to be watched by the TWDT. By watching the Idle Tasks of each CPU, the TWDT can detect instances of tasks running for a prolonged period of time wihtout yielding. This can be an indicator of poorly written code that spinloops on a peripheral, or a task that is stuck in an infinite loop.

The TWDT is built around the Hardware Watchdog Timer in Timer Group 0. When a timeout occurs, an interrupt is triggered. Users can redefine the function esp_task_wdt_isr_user_handler in the user code, in order to receive the timeout event and handle it differently.

---

Ex1 

first exercice is a seeing a watchdog in action.

we create a task that never blocks or yields. the scheduler cannot run other tasks, the idle task won’t run and the Task WDT fires.

First we enable the task watchodog through menuconfig

Run:

```bash
idf.py menuconfig

```

Go to:

```
Component config
  → ESP System Settings
```

and enable Task Watchdog timer

then we write our bad task

```css
void bad_task(void *pvParameters) {
  ESP_LOGI("BAD_TASK", "Bad task started");

  // Add THIS task to the Task Watchdog
  esp_task_wdt_add(NULL);

  while (1) {
    // Infinite loop with NO delay, NO yield
    // This will starve the scheduler
  }
}
```

and create it in app_main

```css
void app_main(void) {
  ESP_LOGI("MAIN", "Starting watchdog demo");

  xTaskCreate(bad_task, "bad_task", 2048, NULL, 5, NULL);
}

```

you get this log

```css
E (5279) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (5279) task_wdt:  - bad_task (CPU 0/1)
E (5279) task_wdt:  - IDLE0 (CPU 0)
E (5279) task_wdt: Tasks currently running:
E (5279) task_wdt: CPU 0: bad_task
E (5279) task_wdt: CPU 1: IDLE1
E (5279) task_wdt: Print CPU 0 (current core) backtrace
```

Fair enough, it says that neither bad_task nor IDLE0 have reset the watchdog. However the esp has not rebooted

even though i set the panic handller behabiour to “Print registers and reboot”

And that s because i have not enabled this “[ ]         Invoke panic handler on Task Watchdog timeout”. When enabled the esp will reboot.

one note, if we coment out this line in out bad task    esp_task_wdt_add(NULL); the wdt woould still fire.

And we’ll have this log

```css
Task watchdog got triggered.
- IDLE0 (CPU 0)
```

That’s because esp-idf monitor idle tasks (IDLE0/IDLE1) by default. these tasks are the scheduler’s hearbeat. So in some sense, the idle task pets the watchdog for system health checking. while tasks we add to the watchdog are for responsibility tracking.

let’s do exercice 2 to see how feeding the dog can hide bugs.

---

ex2

We’ll reuse your Exercise 1 setup and only change **the loop behavior**.

```css
void bad_task(void *pvParameters) {
  esp_task_wdt_add(NULL);

  while (1) {
    esp_task_wdt_reset(); // Feed the watchdog
  }
}
```

- `bad_task` keeps **resetting its own watchdog timer** continuously. But the **scheduler is still starved,** IDLE0 (CPU 0) cannot run because CPU 0 is stuck in the tight loop.

Even if you **manually feed the watchdog**, it **cannot prevent detection of scheduler starvation** because:

- Some tasks (like idle tasks) are **implicitly critical** for system health
- Resetting only *your own task* doesn’t make the system correct

So takeaway is that The watchdog is a *safety net*, not a substitute for correct scheduling. Feeding yourself won’t save a blocked system.
Now let’s check out watchdog timers for interrupts

---

ex3
First, a reminder

- **Task WDT** → monitors tasks and idle tasks, detects scheduler starvation.
- **Interrupt WDT** → monitors ISRs (Interrupt Service Routines) to ensure **they don’t block too long**.

Long or blocking ISRs are dangerous because

**they prevent context switching and RTOS tasks from running**

ESP32 has a **separate interrupt watchdog** to catch this.

Let’s configure an external interrupt to our system with a button press.

we first init our button

```css
void init_button() {
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << BUTTON_GPIO),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = 1, // enable internal pull-up
      .pull_down_en = 0,
      .intr_type = GPIO_INTR_NEGEDGE // falling edge (button press)
  };
  gpio_config(&io_conf);

  // Install ISR service
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
}
```

This button is wired between the GND and the GPIO4.

and the ISR is triggered on falling edge.

This would be our bad interrup

```css
void IRAM_ATTR button_isr_handler(void *arg) {
  // INTENTIONALLY BAD: long blocking loop
   ESP_LOGI(TAG, "ISR fired");
  for (volatile int i = 0; i < 100000000; i++) {
    // do nothing, just waste cycles
  }
}
```

And this would be our main

```c
void app_main(void) {
  ESP_LOGI("MAIN", "Starting watchdog demo");

  /*  ========== Task Watchdog Timer ========== */
  // xTaskCreate(bad_task, "bad_task", 2048, NULL, 5, NULL);

  /*  ========== Interrupt Watchdog Timer ========== */
  init_button();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Main task alive");
  }
}
```

we build and flash. and when we press our button, the board is rebooted but not for the reason we expect.

we get this kind of log

```css
abort() was called at PC 0x40082377
lock_acquire_generic ...
_vfprintf_r ...
esp_log_va ...

```

Actually, **`ESP_LOGI` is *not safe to call from ISRs.*** 

ESP-IDF logging functions ultimately call `vprintf`/`newlib` locks. 

The abort() is exactly what happens when the ISR tries to log: the logger tries to acquire a mutex, but **mutexes cannot be used inside an ISR** → triggers **abort()**.
Once we remove our ESP_LOGI from the ISR handler and rebuild, we can finally see the crash we’ve been waiting for.

```css
Guru Meditation Error: Core  0 panic'ed (Interrupt wdt timeout on CPU0).
```

If you look around in menuconfig, you’ll fid the configuration for this timer

```css
[*] Interrupt watchdog
(300)   Interrupt watchdog timeout (ms)  
```

In my case it is enabled and set at 300ms timeout by default.