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

### Day 2 : Data Logging & Signal Visualization

**End-of-day deliverables (non-negotiable)**

By the end of Day 2, you should have:

- IMU running at **≥100 Hz**
- Logged data for **4 motion classes**
- At least **one plot per motion**
- Clear intuition:
    - “this is noise”
    - “this is vibration”
    - “this is impact”

**Task 1 : Increase sampling rate**

Motion classification needs **time resolution,** 2 Hz is useless for vibration & impact.

- we’ll opt for **100 Hz** (safe, simple)

Change delay to:

```c
vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz

```

**Task 2 : Print Signal Magnitude**

We will:

1. Compute **accel magnitude**
2. Compute **gyro magnitude**
3. Attach a **timestamp**
4. Print everything in **CSV format**

We’ll first include math support.

```c
#include <math.h>
```

Then we’ll compute the magnitude inside  the while loop after scaling.

```c
// Accelerometer magnitude (in g)
float accel_mag = sqrtf(
    ax_g * ax_g +
    ay_g * ay_g +
    az_g * az_g
);

// Gyroscope magnitude (in °/s)
float gyro_mag = sqrtf(
    gx_dps * gx_dps +
    gy_dps * gy_dps +
    gz_dps * gz_dps
);
```

We add a time stamp

```c
#include "esp_timer.h" // add this on top of your file
int64_t timestamp_ms = esp_timer_get_time() / 1000;

```

Finally we print our data in CSV format

```c
printf("%lld,%.4f,%.4f\n",
       timestamp_ms,
       accel_mag,
       gyro_mag);
```

Example output:

```c
34710,1.0352,3.1577
34720,1.0374,3.3325
34730,1.0390,3.3164
34740,1.0375,3.3658
34750,1.0379,3.2720
```

**Task 3 : Log Data to CSV**

We’ll log **raw motion data** so we can *see* the signals before doing any DSP or classification.
We will record **4 CSV files**, one per motion class.

Recording Procedure:

For **each motion class**:

Stationary

- Place IMU on table
- Do not touch
- Record **10–15 s**

 Slow movement

- Gentle hand motion
- No shaking
- 10–15 s

Fast movement / vibration

- Shake continuously
- Same intensity
- 10–15 s

Impact / sudden stop

- Short taps on table
- Leave pauses between taps
- 10–15 s

For this, we’ll create a **single FreeRTOS task** that:

1. Logs IMU data for **15 seconds**
2. Prints:
    
    ```
    ---done---
    ```
    
3. Waits **5 seconds**
4. Repeats this **4 times total**
5. Then **stops logging forever**

```c
void imu_logger_task(void *arg)
{
    const int LOG_TIME_MS   = 15000;
    const int PAUSE_TIME_MS = 5000;
    const int SEGMENTS      = 4;

    for (int seg = 0; seg < SEGMENTS; seg++) {

        int64_t start_time = esp_timer_get_time() / 1000;

        while ((esp_timer_get_time() / 1000 - start_time) < LOG_TIME_MS) {

            // --- Read IMU here ---
            // ax, ay, az in g
            // gx, gy, gz in deg/s

            float accel_mag = sqrtf(ax*ax + ay*ay + az*az);
            float gyro_mag  = sqrtf(gx*gx + gy*gy + gz*gz);
            int64_t time_ms = esp_timer_get_time() / 1000;

            // CSV output
            printf("%lld,%.3f,%.3f\n", time_ms, accel_mag, gyro_mag);

            vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
        }

        printf("---done---\n");

        if (seg < SEGMENTS - 1) {
            vTaskDelay(pdMS_TO_TICKS(PAUSE_TIME_MS));
        }
    }

    // Stop task forever
    vTaskDelete(NULL);
}

```

Before starting the task:

```c
esp_log_level_set("*", ESP_LOG_NONE);
```

This ensures **pure CSV output**.

For each block:

- Copy **from first CSV line**
- Stop at `--done---`
- Paste into one CSV file

The resulting csv files are saved under the motion_data folder where you’ll also find the python file plot_motion.py

```c
import csv
import matplotlib.pyplot as plt

def load_csv(filename):
    time = []
    accel = []
    gyro = []

    with open(filename, 'r') as f:
        reader = csv.reader(f)
        header = next(reader)  # skip header

        for row in reader:
            if len(row) != 3:
                continue

            t, a, g = row
            time.append(float(t) / 1000.0)   # ms → seconds
            accel.append(float(a))
            gyro.append(float(g))

    return time, accel, gyro

def plot_motion(filename, title):
    time, accel, gyro = load_csv(filename)

    plt.figure(figsize=(10, 6))

    plt.plot(time, accel, label="Accel magnitude (g)")
    plt.plot(time, gyro, label="Gyro magnitude (°/s)")

    plt.xlabel("Time (s)")
    plt.ylabel("Magnitude")
    plt.title(title)
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    files = {
        "stationary.csv": "Stationary",
        "slow.csv": "Slow movement",
        "fast.csv": "Fast movement / vibration",
        "impact.csv": "Impact / sudden stop",
    }

    for filename, title in files.items():
        plot_motion(filename, title)

```

To run it, make sure matplotlib is installed by running 

```bash
pip show matplotlib
```

Once the .py works, you’ll get 4 graphs depicting the variation of the acceleration and rotation magnitudes (in g and dps).

### Day3: Preprocessing

**End-of-day deliverables (non-negotiable)**

By the end of Day 3, we should have:

- For each motion CSV:
    - an accelerometer signal without the gravity component
    - a filtered signal (low pass)
    - a plot of raw vs cleaned signal

**Recap**

For the accelerometer magnitude, the measured signal is

```c
|a| = motion + gravity + noise
```

We want **motion only.**

Our strategy is simple. We remove the gravity component by substracting the mean (eliminate the DC) and remove the noise by applying a low-pass filter.

**Python Preprocessing Script**

We create a new file: preprocess_motion.py
We run this scirpt for each csv file and we’ll get the folllowing processed signals:

![Slow Signal : Filtered vs Raw](attachment:4a0d3a51-f09b-4914-84d0-267dd5a742ac:slow_filtered_vs_raw.png)

Slow Signal : Filtered vs Raw

![Stationary Signal : Filtered vs Raw](attachment:aa41c2e1-5f10-427f-bfc8-3f83118227db:stationary_filtered_vs_raw.png)

Stationary Signal : Filtered vs Raw

![Vibration Signal : Filtered vs Raw](attachment:38b67bb4-4249-42a6-8dbf-f1fc6cb93564:vibration_filtered_vs_raw.png)

Vibration Signal : Filtered vs Raw

![Tap Signal : Filtered vs Raw](attachment:9b2b43e9-dd05-4536-8a8b-0e4f47e4767e:impact_filtered_vs_raw.png)

Tap Signal : Filtered vs Raw

We oberve clearly in the accelerometer signal that the gravity is removed, the mean of the filtered signal is near 0.

For both signals (accelerometer and gyroscope), the oscillations have lower amplitude, yet the spikes are not flattened.

### Day4 : Feature Extraction

**End-of-Day Deliverables**

By the end of today, you must have:

- Feature vectors computed for all 4 motion types
- Features saved in a CSV file
- Clear numeric differences between motion classes
- Intuition: “This feature separates X from Y”

We’ll transform each motion signal into feauture vectores per time window.

So instead of having **1000** **samples of waveform**, we’ll have **20 windows → 20 rows of numeric features**

Let’ understand the terms we’re gonna use.

When :

- the sampling rate = 100 Hz
- and we choose a window size of 0.5 seconds

→ We’ll habe 50 samples per window and 20 windows in total for 1000 samples.

For each window, we capture

| Feature | Meaning |
| --- | --- |
| RMS | Energy / intensity |
| Variance | Variability |
| Peak | Sudden spikes |
| Zero-crossing rate | Oscillation frequency |

RMS = how strong is the movement?

- Stationary → very low
- Fast vibration → high

Variance = how much does is change?

- Smooth slow movement → low variance
- Shaking → high variance

Peak Value = was there a sudden event?

- Impact → very high peak
- Slow movement → low peak

Zero-crossing rate = how often does it oscillate?

- Vibration → many crossings
- Stationary → almost none

**Implementation**

```c
import numpy as np
import csv
import os
from scipy.signal import butter, filtfilt

# -----------------------------
# Parameters
# -----------------------------
FS = 100                 # Sampling frequency (Hz)
WINDOW_SIZE = 0.5        # seconds
SAMPLES_PER_WINDOW = int(FS * WINDOW_SIZE)
CUTOFF = 10              # Low-pass cutoff frequency

DATA_FOLDER = "motion_data"
OUTPUT_FILE = "features.csv"

# -----------------------------
# Low-pass filter
# -----------------------------
def lowpass(signal, cutoff, fs, order=4):
    nyq = 0.5 * fs
    norm_cutoff = cutoff / nyq
    b, a = butter(order, norm_cutoff, btype="low")
    return filtfilt(b, a, signal)

# -----------------------------
# Feature functions
# -----------------------------
def compute_features(window):
    rms = np.sqrt(np.mean(window**2))
    variance = np.var(window)
    peak = np.max(np.abs(window))

    # zero-crossing rate
    zero_crossings = np.where(np.diff(np.sign(window)))[0]
    zcr = len(zero_crossings) / len(window)

    return rms, variance, peak, zcr

# -----------------------------
# Process one file
# -----------------------------
def process_file(filepath, label):
    time = []
    accel = []
    gyro = []

    with open(filepath, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            t, a, g = row
            time.append(float(t))
            accel.append(float(a))
            gyro.append(float(g))

    accel = np.array(accel)
    gyro = np.array(gyro)

    # ---- Preprocessing (Day 3 pipeline) ----
    accel_motion = accel - np.mean(accel)
    accel_filtered = lowpass(accel_motion, CUTOFF, FS)
    gyro_filtered = lowpass(gyro, CUTOFF, FS)

    features = []

    # ---- Windowing ----
    for start in range(0, len(accel_filtered) - SAMPLES_PER_WINDOW, SAMPLES_PER_WINDOW):
        end = start + SAMPLES_PER_WINDOW

        acc_win = accel_filtered[start:end]
        gyro_win = gyro_filtered[start:end]

        acc_features = compute_features(acc_win)
        gyro_features = compute_features(gyro_win)

        features.append([
            *acc_features,
            *gyro_features,
            label
        ])

    return features

# -----------------------------
# Main
# -----------------------------
all_features = []

motion_files = {
    "stationary.csv": "stationary",
    "slow.csv": "slow",
    "fast.csv": "fast",
    "impact.csv": "impact"
}

for filename, label in motion_files.items():
    filepath = os.path.join(DATA_FOLDER, filename)
    feats = process_file(filepath, label)
    all_features.extend(feats)

# Save to CSV
with open(OUTPUT_FILE, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
        "acc_rms", "acc_var", "acc_peak", "acc_zcr",
        "gyro_rms", "gyro_var", "gyro_peak", "gyro_zcr",
        "label"
    ]
    writer.writerows(all_features)

print("Feature extraction complete.")
print(f"Saved to {OUTPUT_FILE}")
```

After running this code, open features.csv 

You should see rows like this 

```c
0.02,0.0001,0.05,0.02,1.3,0.5,4.0,0.1,stationary
```

Congrats! you just created feautre vectors!

Let’s draw some plots to understand these numbers.

Run 

```c
python analyze_features.py
```

This code produces 6 “boxplots”.

First we should learn how to read a boxplot

In our case, each rectangle represents one motion class.

Inside each box:

```css
   ───────         ← top whisker
      │
   ┌────────┐
   │        │
   │        │
   └────────┘
      │
   ───────         ← bottom whisker
```

**The box** represents the **middle 50% of your data**.

- Bottom of box → 25th percentile
- Top of box → 75th percentile

This tells you: “Where most of the values lie.”

**The horizontal line** inside the box = **Median**

This is the middle value.

If you want a single representative number per class → look at the median.

**The Whiskers**

The vertical lines extending from the box show how far the data spreads (excluding extreme outliers)

They indicate variability.

**Tiny Circles**

These are **outliers, v**alues far away from the normal distribution.

![acc_peak.png](attachment:bab9aae6-7d9b-4f58-8e1f-1d960c6fcc06:acc_peak.png)

![acc_rms.png](attachment:c6711e73-2cb9-49d4-9f27-50c2fec4d41b:acc_rms.png)

![acc_var.png](attachment:48a83016-dad5-4aaa-b33a-1ec409bd104d:acc_var.png)

![gyro_var.png](attachment:df625b34-bce9-4c0b-a69d-c5e6b98689a2:gyro_var.png)

![gyro_peak.png](attachment:473c3e76-557d-432c-8c0a-89998f4d4b36:gyro_peak.png)

![gyro_rms.png](attachment:143ce125-2c5f-469f-a8a7-b2825e5e4cf1:gyro_rms.png)

- many outliers in the gyro’s peak and rms values.
- the vibration always has the largest box, it never overlapst with other boxes.
- the stationary’s box is always flat compared to other signals.
- what clearly disctincts the slow motion from the stationary motion is : the slow box is much wider in the gyro’s peak and the gyro’s rms plots and the whiskers are also wider.

If i had to choose only 2 features for classification, I’d choose gyro_peak and acc_rms.