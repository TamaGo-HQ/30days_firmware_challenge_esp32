import numpy as np
import matplotlib.pyplot as plt
import csv
from scipy.signal import butter, filtfilt

# ----------------------------
# Parameters
# ----------------------------
FS = 100            # sampling frequency in Hz (adjust if needed)
CUTOFF = 10         # low-pass cutoff frequency (Hz)

FILE = "motion_data/tap.csv"   # change file to inspect others

# ----------------------------
# Load CSV
# ----------------------------
time_ms = []
accel_mag = []
gyro_mag = []

with open(FILE, "r") as f:
    reader = csv.reader(f)
    for row in reader:
        t, a, g = row
        time_ms.append(float(t))
        accel_mag.append(float(a))
        gyro_mag.append(float(g))

time_ms = np.array(time_ms)
accel_mag = np.array(accel_mag)
gyro_mag = np.array(gyro_mag)

time_s = (time_ms - time_ms[0]) / 1000.0

# ----------------------------
# Gravity removal
# ----------------------------
accel_motion = accel_mag - np.mean(accel_mag)

# ----------------------------
# Low-pass filter
# ----------------------------
def lowpass(signal, cutoff, fs, order=4):
    nyq = 0.5 * fs
    norm_cutoff = cutoff / nyq
    b, a = butter(order, norm_cutoff, btype="low")
    return filtfilt(b, a, signal)

accel_filtered = lowpass(accel_motion, CUTOFF, FS)
gyro_filtered = lowpass(gyro_mag, CUTOFF, FS)

# ----------------------------
# Plot results
# ----------------------------
plt.figure(figsize=(10, 6))

plt.subplot(2, 1, 1)
plt.plot(time_s, accel_mag, label="Raw |a|", alpha=0.5)
plt.plot(time_s, accel_filtered, label="Filtered motion |a|")
plt.title("Accelerometer magnitude")
plt.xlabel("Time (s)")
plt.ylabel("g")
plt.legend()

plt.subplot(2, 1, 2)
plt.plot(time_s, gyro_mag, label="Raw |ω|", alpha=0.5)
plt.plot(time_s, gyro_filtered, label="Filtered |ω|")
plt.title("Gyroscope magnitude")
plt.xlabel("Time (s)")
plt.ylabel("deg/s")
plt.legend()

plt.tight_layout()
plt.show()
