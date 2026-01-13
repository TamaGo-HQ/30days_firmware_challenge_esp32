## Scope Lock

This project is the second milestone of the 30 days firmware challenge.

The final deliverable is a working system and its documentation.

The application is to be run on an ESP32 board and built with ESPIDF. It’s composed of one IMU sensor and one Ultrasonic sensor. The board is to powerd by the PC through the UART board.

**Requirements:**

- Sampling rate:
    - Ultrasonic: 500ms
    - IMU: 500ms
- Output:
    - UART log (ESP_LOG)
- Logger format:
    - Timestamp + sensor ID + values
- Error handling:
    - Timeout → log warning

## High-Level Architecture Design

### Task Model

The following tasks will run

| Task | Responsibility | Priority |
| --- | --- | --- |
| `ultrasonic_task` | Read ultrasonic | High |
| `imu_task` | Read IMU #1 | High |
| `aggregator_task` | Collect & timestamp sensor data | Medium |
| `logger_task` | Serialize & log data | Low |

### Communication Design

**Queues**

- Sensor → Aggregator
- Aggregator → Logger

**Message** 

```c
typedef enum { 
  SENSOR_IMU,
  SENSOR_ULTRASONIC 
}sensor_type_t;

typedef struct {
  sensor_type_t type;
  int64_t timestamp; // acquisition timestamp
  float data[4];
} sensor_msg_t;
```

### RTOS Decisions

Data flow

```css
[ IMU Task ]          \
                      --> [ Aggregator ] --> [ Logger ]
[ Ultrasonic Task ]  /

```

Task Priorities

| Task | Priority |
| --- | --- |
| IMU Task | 8 |
| Ultrasonic Task | 8 |
| Aggregator Task | 7 |
| Logger Task | 1 |

The IMU and Ultrasonic Task have the same sampling rate for simplicity and hence have been assigned the same priority.

Gaps are left for potential growth.

Communication objects

| Object | Purpose |
| --- | --- |
| `sensor_to_agg_q` | Sensor data buffer |
| `agg_to_log_q` | Logging buffer |

No syncronization objects required as Queeues already synchronize our system

### Project Skeleton

```css
main/
 ├── main.c
 ├── tasks/
 │    ├── imu_task.c / .h
 │    ├── ultrasonic_task.c / .h
 │    ├── aggregator_task.c / .h
 │    └── logger_task.c / .h
 ├── sensors/
 │    ├── imu_driver.c / .h
 │    └── ultrasonic_driver.c / .h
 └── common/
      └── messages.h
```

### Sensor Task Implementation

Each sensor task should:

- Read sensor
- Add system timestamp
- Send to aggregator queue
- `vTaskDelay()`

Key RTOS practices to show:

- Timeout on queue send
- Error logging on failure

Deliverable

- Sensor tasks pushing real data
- Aggregator receiving real messages

### Aggregator Task Implementation

Reponsibilities

- Receive messages from sensors
- Forward to logger queue

Important design decision

- Blocking receive with timeout
- No busy loops

Deliverable

- Clean aggregation logic
- Logs showing order

### Logger Task Implementation

Responsibilities

- Receive aggregated data
- Format output
- Log

Deliverable

- Clean UART logs
- Logger never blocks system

### Basic testing

Tests to perform

- Unplug each sensor → system continues?

Deliverable

- **Testing section in README**
    - What you tested
    - What happened
    - What you learned

---

Testing:

Currently 2 cases are tested

**Unpluging the IMU Sensor**

```css
I (13821) LOGGER: [IMU] ts=13559881 | ax=2.87 ay=-10.31 az=6.11
I (14131) LOGGER: [ULTRA] ts=13819124 | distance=1019.77 cm
E (14321) IMU_DRIVER: Failed to select GYRO_XOUT_H register
W (14321) IMU_TASK: Failed to read IMU
I (14681) LOGGER: [ULTRA] ts=14369124 | distance=1019.74 cm
E (14821) IMU_DRIVER: Failed to select GYRO_XOUT_H register
W (14821) IMU_TASK: Failed to read IMU
I (15231) LOGGER: [ULTRA] ts=14919124 | distance=1019.77 cm
E (15321) IMU_DRIVER: Failed to select GYRO_XOUT_H register
W (15321) IMU_TASK: Failed to read IMU
```

we get an error log and the Ultrasonic sensor data is still logged.

However when we plug it back all IMI data is zeroed

```css
I (19321) LOGGER: [IMU] ts=19059877 | ax=0.00 ay=0.00 az=0.00
```

and a reboot of the board is required.

**Unpluggin the ultrasonic**

```css
I (7321) LOGGER: [IMU] ts=7059884 | ax=-4.00 ay=-0.83 az=1.21
I (7531) LOGGER: [ULTRA] ts=7219124 | distance=1020.67 cm
I (7821) LOGGER: [IMU] ts=7559880 | ax=-14.42 ay=5.05 az=-14.24
W (8131) ULTRASON: Echo timeout
W (8131) ULTRASON_TASK: Failed to read Ultrason
I (8321) LOGGER: [IMU] ts=8059880 | ax=-7.50 ay=0.17 az=-0.80
W (8731) ULTRASON: Echo timeout
W (8731) ULTRASON_TASK: Failed to read Ultrason
```

Similarly, we get an error log and the IMU data logging works as normaly.

In this case the sensor work normaly after replugging without needing to reboot the system.

The IMU sensor communicates via I²C and is not hot-plug tolerant.

Disconnecting the IMU during runtime may place the I²C bus in an invalid state, requiring a system reboot or bus reinitialization.

GPIO-based sensors like the ultrasonic sensor recover automatically.

Conclusion:

In this project, we followed a clear development process. we began by identifying the scope of the project and goal of working on it. This reminds us to that we have limited time and resources and should not set requirements we cannot meet.

We then set a clear plan with milestones and a deliverable for each milestone.

Most importantly we did not negligate the design step of the project. This saves time as we lay the data and code flow of the applications and helps us decide what solutions to addapt for each component.

This is also the base of any project and it helped with keeping a birds eye view of the project even when coding low level parts like sensor drivers.

Lastly this project lacks lots of features and testing and one thing to note is that the sensor drivers were written with ai assistance as it was not the main focus of this project.

Books read when working on this 

- Embedded RTOS Desing by Colin Walls @2021 Elsevier (first three chapters)

Bis Bald