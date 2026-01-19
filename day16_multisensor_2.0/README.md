This project is an update of the multisensor 1.0 project completed last week.

The last version was a means to apply aqcuired knowledge about task scheduling and inter task communication.

This version will include applications of NVS memory, watchdogs, low-power modes and MISRA-aware code style.

### Requirements

**Functional Requirements**

- Read ultrasonic distance periodicaly
- Read IMU acceleration periodicaly
- Blink LED when system is awake
- Enter deep sleep between cycles

**Non-Functional Requirements**

- Must survive sensor disconnection

### Architecure & Design

Software Architecture

app_main

- init
- load config
- create tasks

task : sensor_task

- read sensor
- feed watchodog
- signal success/failure

task : sleep_manager

- decide sleep duration

task : led_task

- turns on/off LED

task : logger

- logs sensor data

**State based design**

```css
INIT → LOAD_CONFIG → READ_SENSORS → HANDLE_ERROR → SLEEP
											    ^--------------------------|				
```

**Low power strategy**

We’ll opt for deep sleep as there is no need for light sleep.

**Development tasks**

We start with multisensor 1.0 as the base as the sensor tasks and their inter communication is already set and satisfactory. We’ll add the new features on that already developed code.

- [x]  add configuration in NVS
- configuration struct is now defined in NVS driver header
- default configuration values are now defined in NVS driver source
- shoud load and save operation be critical sections?
- for now only reads default configuration, does not take user input
- [x]  add low power feature
- only the logging is implemented as an independent RTOS task
- the sensor readings and sleeping are implemented as simple functions inside app_main()
- [x]  LED task feature
- LED stays on while the chip is awake

**Design Notes**

Continuous system services (logging, aggregation) are implemented as FreeRTOS tasks.

One-shot operations (sensor sampling, configuration loading, power-state transitions) are implemented as sequential functions executed during the active phase before deep sleep.

---

Next version

- [ ]  add user input for configuration
- [ ]  send sensor data wirelessly