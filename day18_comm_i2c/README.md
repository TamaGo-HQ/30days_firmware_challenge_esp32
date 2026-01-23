## Research Notes

https://www.electronicshub.org/basics-i2c-communication/?utm_source=chatgpt.com

https://www.geeksforgeeks.org/computer-organization-architecture/i2c-communication-protocol/

https://www.engineersgarage.com/i2cinter-integrated-circuit-twitwo-wire-interface/?utm_source=chatgpt.com

I2C stands for **Inter-Integrated Circuit.**

The I2C bus was developed in the early 1980’s by Philips Semiconductors. Its original purpose was to provide an easy way to connect a CPU to peripheral chips in a TV-set.

The research done by Philips Labs in Eindhoven (The Netherlands) to overcome these problems resulted in a 2-wire communication bus called the I2C bus. I2C is an acronym for Inter-IC bus. Its name literally explains its purpose: to provide a communication link between Integrated Circuits.

Owing to its simplicity, it is widely adopted for short-distance communication between microcontrollers and sensor arrays, displays, IoT devices, EEPROMs etc.

### **Features**

- Only two common bus lines (wires) are required to control any device/IC on the I2C network
- No need of prior agreement on data transfer rate like in [UART communication](https://www.electronicshub.org/basics-uart-communication/). So the data transfer speed can be adjusted whenever required
- Simple mechanism for validation of data transferred
- Uses 7-bit addressing system to target a specific device/IC on the I2C bus
- I2C networks are easy to scale. New devices can simply be connected to the two common I2C bus lines

### **Hardware**

**The physical I2C Bus**

I2C Bus (Interface wires) consists of just two bi-directional lines : Serial Clock Line (SCL) and Serial Data Line (SDA).

The data to be transferred is sent through the SDA wire and is synchronized with the clock signal from SCL. All the devices/ICs on the I2C network are connected to the same SCL and SDA lines as shown below:

![image.png](attachment:418be96a-4f82-4fd3-ba48-0c760297d0ed:image.png)

For each clock pulse one bit of data is transferred. The SDA signal can only change when the SCL signal is low. When the clock is high the data should be stable.

So both the I2C bus lines (SDA, SCL) are operated as open drain drivers, meaning they are active low. For this, a pull up resistor is used for each bus line, to keep them high by default.

![image.png](attachment:5841b3db-2457-4e63-b121-c3696cc2d7cf:image.png)

Any device/IC on the I2C network can drive SDA and SCL low, but they cannot drive them high.

The reason for using an open-drain system is that there will be no chances of shorting, which might happen when one device tries to pull the line high and some other device tries to pull the line low.

**Master and Slave Devices**

The devices connected to the I2C bus are categorized as either masters or slaves. At any instant of time only a single master stays active on the I2C bus. It controls the SCL clock line and decides what operation is to be done on the SDA data line.

All the devices that respond to instructions from this master device are slaves. For differentiating between multiple slave devices connected to the same I2C bus, each slave device is physically assigned a permanent 7-bit address.

When a master device wants to transfer data to or from a slave device, it specifies this particular slave device address on the SDA line and then proceeds with the transfer. So effectively communication takes place between the master device and a particular slave device.

All the other slave devices doesn’t respond unless their address is specified by the master device on the SDA line.

![image.png](attachment:c8b5d516-adaf-4b0d-953a-84288821a2b1:image.png)

### Data Transfer Protocol

Data is transferred between the master device and slave devices through a single SDA data line.

**Start Condition**

Whenever a master device/IC decides to start a transaction, it switches the SDA line from high to low before the SCL line switches high to low.

Once a start condition is sent by the master device, all the slave devices get active even if they are in sleep mode, and wait for the address bits.

**Addressing the Slave**

It comprises of 7 bits and are filled with the address of slave device to/from which the master device needs send/receive data. All the slave devices on the I2C bus compare these address bits with their address.

**Read/Write Bit**

This bit specifies the direction of data transfer. If the master device/IC need to send data to a slave device, this bit is set to ‘0’. If the master IC needs to receive data from the slave device, it is set to ‘1’.

**ACK/NACK Bit**

The addressed slave device responds by pulling the SDA line low during the next clock pulse (SCL). This confirms that the slave is ready to communicate.

**Data Transmission**

The master or slave (depending on the read/write operation) sends data in 8-bit chunks. After each byte, an ACK is sent to confirm that the data has been received successfully.

**Stop Condition**

When the transmission is complete, the master sends a stop condition by releasing the SDA line to high while the SCL line is high. This signals that the communication session has ended.

![image.png](attachment:c9cac75c-b510-40b9-8af9-46d04b916f4e:image.png)

### How I2C Communication Practically Works?

An I2C communication/transaction is initiated by a master device either to send data to a slave device or to receive data from it. Let us learn about working of both the scenarios in detail.

**Sending Data to a Slave device**

- The master device sends the start condition
- The master device sends the 7 address bits which corresponds to the slave device to be targeted
- The master device sets the Read/Write bit to ‘0’, which signifies a write
- Now two scenarios are possible
    - If no slave device matches with the address sent by the master device, the next ACK/NACK bit stays at ‘1’ (default). This signals the master device that the slave device identification is unsuccessful. The master clock will end the current transaction by sending a Stop condition or a new Start condition
    - If a slave device exists with the same address as the one specified by the master device, the slave device sets the ACK/NACK bit to ‘0’, which signals the master device that a slave device is successfully targeted
- If the data is successfully received by the slave device, it sets the ACK/NACK bit to ‘0’, which signals the master device to continue
- The previous two steps are repeated until all the data is transferred
- After all the data is sent to the slave device, the master device sends the Stop condition which signals all the slave devices that the current transaction has ended

The below figure represents the overall data bits sent on the SDA line and the device that controls each of them:

![image.png](attachment:c8bdbae8-2b1a-491e-997b-24686adc8941:image.png)

**Reading Data from a Slave Device**

The sequence of operations remain the same as in previous scenario except for the following:

- The master device sets the Read/Write bit to ‘1’ instead of ‘0’ which signals the targeted slave device that the master device is expecting data from it
- The 8 bits corresponding to the data block are sent by the slave device and the ACK/NACK bit is set by the master device
- Once the required data is received by the master device, it sends a NACK bit. Then the slave device stops sending data and releases the SDA line

If the master device to read data from specific internal location of a slave device, it first sends the location data to the slave device using the steps in previous scenario. It then starts the process of reading data with a repeated start condition.

![image.png](attachment:fd79062c-f4ec-4d04-b05a-7673b889740e:image.png)

**Concept of clock stretching**

Let say the master device started a transaction and sent address bits of a particular slave device followed by a Read bit of ‘1’. The specific slave device needs to send an ACK bit, immediately followed by data.

But if the slave device needs some time to fetch and send data to master device, during this gap, the master device will think that the slave device is sending some data.

To prevent this, the slave device holds the SCL clock line low until it is ready to transfer data bits. By doing this, the slave device signals the master device to wait for data bits until the clock line is released.

**Advantages of I2C Communication Protocol**

- Can be configured in multi-master mode.
- Complexity is reduced because it uses only 2 bi-directional lines (unlike SPI Communication).
- Cost-efficient.
- It uses ACK/NACK feature due to which it has improved error handling capabilities.
- **Fewer Wires:** Only two wires are needed, making it easier to set up.
- **Multiple Devices**: You can connect many devices to the same bus.
- **Simple Communication:** It’s relatively easy to program and use.

**Disadvantages of I2C Communication Protocol**

- **Speed Limitations:** I2C is slower compared to some other protocols like SPI.
- **Distance:** It’s not suitable for long-distance communication.
- Half-duplex communication is used in the I2C communication protocol.

Check this video by Rohde & Schwarz to lock it in visually [Understanding I2C](https://www.youtube.com/watch?v=CAvawEcxoPU)

## Exercices

For the following secition, we’ll be using an IMU sensor to understand the I2C communcation protocol on a practical level. Use may use whatever sensor you have that relies on I2C communication.

### Exercice 1 : I2C address scanning

**Goal**

Find our IMU sensor on the bus and understand slave addressing

**Deliverable**

- Table of detected addresses
- Note which matches your IMU datasheet

**Step 0 : Wiring**

- **SDA** → connect to IMU SDA pin
- **SCL** → connect to IMU SCL pin
- Power your IMU correctly (3.3 V or 5 V, check datasheet)

For faster dev, we’ll use the ESP-IDF I2C driver instead of big-banging

### Step 2 : Bus Scanning

```css
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO           22      // change to your GPIO
#define I2C_MASTER_SDA_IO           21      // change to your GPIO
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ           100000
#define I2C_MASTER_TX_BUF_DISABLE    0
#define I2C_MASTER_RX_BUF_DISABLE    0

static const char *TAG = "I2C_SCAN";

void i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                       I2C_MASTER_RX_BUF_DISABLE,
                       I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t i2c_probe(uint8_t addr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void app_main(void)
{
    i2c_master_init();
    ESP_LOGI(TAG, "Scanning I2C bus...");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        esp_err_t ret = i2c_probe(addr);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Device found at 0x%02X", addr);
        }
        else if (ret == ESP_ERR_TIMEOUT)
        {
            ESP_LOGW(TAG, "Device at 0x%02X timed out", addr);
        }
        else
        {
            ESP_LOGD(TAG, "No device at 0x%02X", addr);
        }
    }
    ESP_LOGI(TAG, "Scan complete.");
}

```

Flash this into your ESP32 and open your monitor. I got this log :

```css
I (298) main_task: Calling app_main()
I (298) I2C_SCAN: Scanning I2C bus...
I (318) I2C_SCAN: Device found at 0x68
I (318) I2C_SCAN: Scan complete.
I (318) main_task: Returned from app_main()
```

Expected. In my sensor’s datasheet there is this indication : AD0 - I2C Address pin. Pulling this pin high or bridging the solder jumper on the back will change the I2C address from 0x68 to 0x69. 

In this experiment i did not use the AD0 pin, so the devices adress is 0x68 by default.

### Exercice 2 : **Read a register from IMU (WHO_AM_I)**

**Goal** 

Perform a datasheet-driven read without libraries. Core learning: how START/STOP, R/W bit, and repeated START work.

**Implementation**

1. Check your sensors Datasheet

From your datasheet, extract the 7-bit I2C adress, WHO_AM_I register adress and the expected WHO_AM_I value

The following link is the MPU-6000/MPU-6050 Register Map document I used.

https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf

```css
IMU I2C address (7-bit): 0x68
WHO_AM_I register: 0x75
Expected value: 0x68
```

1. ESP-IDF I2C transaction implementation

```css
START
→ [ADDR | W]        (master)
→ ACK               (slave)
→ [WHO_AM_I_REG]    (master)
→ ACK               (slave)
→ REPEATED START
→ [ADDR | R]        (master)
→ ACK               (slave)
→ [DATA BYTE]       (slave)
→ NACK              (master)
→ STOP
```

ESP32’s I2C controller operating as master is responsible for establishing communication with I2C slave devices and sending commands to trigger a slave to action, for example, to take a measurement and send the readings back to the master.

For better process organization, the driver provides a container, called a “command link”, that should be populated with a sequence of commands and then passed to the I2C controller for execution.

To better understand this exercice, check out espidf’s documentation.

 https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/api-reference/peripherals/i2c.html#master-write

The following function is the MVP of the code

```css
esp_err_t imu_read_who_am_i(uint8_t *who_am_i)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // START
    i2c_master_start(cmd);

    // Address + Write
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_WRITE, true);

    // Register address
    i2c_master_write_byte(cmd, WHO_AM_I_REG, true);

    // Repeated START
    i2c_master_start(cmd);

    // Address + Read
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_READ, true);

    // Read 1 byte, then NACK
    i2c_master_read_byte(cmd, who_am_i, I2C_MASTER_NACK);

    // STOP
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(
        I2C_MASTER_NUM,
        cmd,
        1000 / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete(cmd);
    return ret;
}

```

Call it from app_main

```css
void app_main(void)
{
    i2c_master_init();

    uint8_t who_am_i = 0;
    esp_err_t ret = imu_read_who_am_i(&who_am_i);

    if (ret == ESP_OK) {
        ESP_LOGI("IMU", "WHO_AM_I = 0x%02X", who_am_i);
    } else {
        ESP_LOGE("IMU", "Failed to read WHO_AM_I (%s)", esp_err_to_name(ret));
    }
}
```

**Results:**

After building and flashing, we get this log

```css
I (288) main_task: Started on CPU0
I (298) main_task: Calling app_main()
I (298) I2C_SCAN: WHO_AM_I = 0x68
I (298) main_task: Returned from app_main()
```

This diagram describes the transaction that just happened

```css
Master: START
Master: 0x68 + W  -------->
Slave:              ACK
Master: 0x75       -------->
Slave:              ACK
Master: RESTART
Master: 0x68 + R  -------->
Slave:              ACK
Slave: 0x68       -------->
Master:             NACK
Master: STOP

```

Let’s try reading multiple bytes from the IMU

### Exercice 3 : Multi-byte register read

**Goal**

Read multiple consecutive IMU registers in **one I²C transaction** using a repeated START.

**Implementation**

The IMU stores **16-bit values** for each axis (X, Y, Z).

- **16 bits** = 2 bytes
- One byte is **High** (H), the other is **Low** (L)

**Datasheet prep**

We need to find the accelerometer registers ,number of bytes and the byte order.

```css
ACCEL_XOUT_H = 0x3B
ACCEL_XOUT_L = 0x3C
ACCEL_YOUT_H = 0x3D
ACCEL_YOUT_L = 0x3E
ACCEL_ZOUT_H = 0x3F
ACCEL_ZOUT_L = 0x40
Read 6 bytes:
XH, XL, YH, YL, ZH, ZL
```

**I2C transaction**

Transaction will follow this process

```css
START
→ ADDR + W
→ ACK
→ ACCEL_START_REG
→ ACK
→ REPEATED START
→ ADDR + R
→ ACK
→ DATA[0] → ACK
→ DATA[1] → ACK
→ DATA[2] → ACK
→ DATA[3] → ACK
→ DATA[4] → ACK
→ DATA[5] → NACK
→ STOP
```

**Implementation**

```css
esp_err_t imu_read_bytes(uint8_t start_reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // START + write phase
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, start_reg, true);

    // Repeated START + read phase
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_READ, true);

    // Read multiple bytes
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);

    // STOP
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(
        I2C_MASTER_NUM,
        cmd,
        1000 / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete(cmd);
    return ret;
}
```

We add this function to wake up the sensor from sleep before reading the values.

```css
void imu_wake_up(void)
{
    uint8_t data = 0x00; // clear sleep bit
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, PWR_MGMT_1, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}
```

We update the app_main()

```css
void app_main(void)
{
    ESP_LOGI(TAG, "Initializing I2C...");
    i2c_master_init();
    imu_wake_up();
    uint8_t raw[READ_LEN];
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGI("I2C_SCAN", "Found device at 0x%02X", addr);
    }
}

    while (1) {
        esp_err_t ret = imu_read_bytes(ACCEL_START_REG, raw, READ_LEN);
        if (ret == ESP_OK) {
            int16_t ax = (raw[0] << 8) | raw[1];
            int16_t ay = (raw[2] << 8) | raw[3];
            int16_t az = (raw[4] << 8) | raw[5];

            float ax_g = (float)ax / 16384.0f;
            float ay_g = (float)ay / 16384.0f;
            float az_g = (float)az / 16384.0f;
            ESP_LOGI(TAG, "AX=%.2f g, AY=%.2f g, AZ=%.2f g", ax_g, ay_g, az_g);
        } else {
            ESP_LOGE(TAG, "Failed to read IMU: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms delay
    }
}

```

**Results**

We get the following logs

```css
I (289) main_task: Started on CPU0
I (299) main_task: Calling app_main()
I (299) I2C_SCAN: Initializing I2C...
I (319) I2C_SCAN: Found device at 0x68
I (319) I2C_SCAN: AX=-0.63 g, AY=-0.66 g, AZ=-0.46 g
I (819) I2C_SCAN: AX=-0.61 g, AY=-0.53 g, AZ=-0.46 g
I (1319) I2C_SCAN: AX=-0.63 g, AY=-0.62 g, AZ=-0.45 g
```

## All done

Today, we 

- Learned **I²C at the signal level**
- Scanned the bus and confirmed the IMU
- Wrote a **manual multi-byte read** using repeated START
- Parsed **raw accelerometer values**

Next time, we’ll add another imu sensor to the bus and play around with it.