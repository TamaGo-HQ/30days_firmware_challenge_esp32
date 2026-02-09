### Day 1 : Sensor Understanding & Setup

**End-of-day deliverables (checklist)**

By the end of Day 1, we should have:

- MPU-6050 detected on I²C bus
- ESP32 reading **raw accelerometer & gyroscope registers**
- Values printed at a **fixed, known sampling rate**
- We understand:
    - what the raw numbers mean
    - how to convert them to **g** and **°/s**

Sensor outputs: 

The MPU-60X0 features three 16-bit analog-to-digital converters (ADCs) for digitizing the gyroscope outputs and three 16-bit ADCs for digitizing the accelerometer outputs.  

For precision tracking of both fast and slow motions, the parts feature a user-programmable gyroscope full-scale range of ±250, ±500, ±1000, and ±2000°/sec (dps) and a user-programmable accelerometer full-scale range of ±2g, ±4g, ±8g, and ±16g.

**Sensor Specifications**

First we should understand the terms mentioned in the sensor documentation.

We’ll use the Gyroscope Spec values as exemples.

You can find them in the Product specification document.

1. Full-Scale Range
    
    What angular speeds can be measured? This depends on the register FS_SEL
    
    | FS_SEL | Range |
    | --- | --- |
    | 0 | ±250 °/s |
    | 1 | ±500 °/s |
    | 2 | ±1000 °/s |
    | 3 | ±2000 °/s |
    
    If your motion exceed the range, there’ll be clipping.
    
2. Sensitivity Scale Factor
    
    How do raw number map to real units?
    
    According to the table, if FS_SEL = 0, then 131 LSB = 1°/s.
    
    So
    
    ```c
    gyro_dps = raw / 131.0;
    ```
    
3. Gyroscope ADC word length 
    
    This is the resolution of the measurement. The output is signed 16-bit. It ranges from −32768 to +32767.
    
4. Zero-Rate Output (ZRO)
    
    This is the Gyro output when NOT rotating. The specs indicate “Initial ZRO tolerance: ±20 °/s”. That means that you may see values like `+8 °/s` even when stationary
    

Now that we understand the terms and raw data we’ll be working with, let’s see the actual data from our sensor.

We use the I2C project (day18) as the base for our project. We previously only read the accelometer data, now we’ll modify the code to read the gyro data too.

We don’t have too much modifications to do, only the while loop in our app_main and the READ_LEN macro. We previously only read 6 bytes, now we’ll read 14 bytes.

<aside>
🫠

Reminder: 

The MPU-6050 data layout is:

0x3B–0x40 : Accel (6 bytes)
0x41–0x42 : Temperature (2 bytes)
0x43–0x48 : Gyro (6 bytes)

</aside>

For the scaling, we are using the defaults (FS_SEL = 0), so  a range of ±250 °/s and the sensibility factor of **131 LSB / °/s.**

So in our code we scale the raw data accordingly.

```c
gyro_dps = raw / 131.0f;
```

Our while loop becomes :

```c
#define READ_LEN 14

while (1) {
    esp_err_t ret = imu_read_bytes(ACCEL_START_REG, raw, READ_LEN);
    if (ret == ESP_OK) {

        // -------- Accelerometer --------
        int16_t ax = (raw[0] << 8) | raw[1];
        int16_t ay = (raw[2] << 8) | raw[3];
        int16_t az = (raw[4] << 8) | raw[5];

        float ax_g = ax / 16384.0f;
        float ay_g = ay / 16384.0f;
        float az_g = az / 16384.0f;

        // -------- Gyroscope --------
        int16_t gx = (raw[8]  << 8) | raw[9];
        int16_t gy = (raw[10] << 8) | raw[11];
        int16_t gz = (raw[12] << 8) | raw[13];

        float gx_dps = gx / 131.0f;
        float gy_dps = gy / 131.0f;
        float gz_dps = gz / 131.0f;

        ESP_LOGI(TAG,
            "A[g]: X=%.2f Y=%.2f Z=%.2f | G[dps]: X=%.2f Y=%.2f Z=%.2f",
            ax_g, ay_g, az_g,
            gx_dps, gy_dps, gz_dps
        );

    } else {
        ESP_LOGE(TAG, "Failed to read IMU: %s", esp_err_to_name(ret));
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // keep same rate for now
}

```

We build and flash and monitor.

When stationary, we get these values

```c
I (319) I2C_SCAN: A[g]: X=1.03 Y=-0.06 Z=-0.01 | G[dps]: X=-3.15 Y=-0.69 Z=0.63
```

For the Accel, one axis (X) ≈ **±1 g while the others** ≈ 0.

For the Gyro, all axes are small non-zero values. 

Try rotating the sensor around one axis at a time and moving it linearly following one axis at a time and Observe the change of the printed values.

If you’re confused as to why the Accel values when stationary are the way they are (only one axis is around 1g and the others are around 0), this goes to the physics of the accelerometer.

Accelerometers are **not designed to measure speed change directly**.

They measure **force per unit mass** acting on a tiny internal mass.

And the **most important constant force acting on that mass is gravity**. Which is why the acceleration is expressed in **multiples of gravity (`g`)**, not in m/s².

At rest on a table:

- True acceleration = 0 m/s²
- Accelerometer output ≈ **+1 g**

Why?

Because the table is **pushing up** on the sensor to counter gravity.

That force is equivalent to **1 g**.

That’s it for today, see you tomorrow.

Resources:

[MPU-6000-Datasheet1.pdf](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)