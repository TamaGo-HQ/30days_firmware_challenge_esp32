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

DATA_FOLDER = ""
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
        next(reader)  # skip header row
        
        for row in reader:
            if len(row) != 3:
                continue

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
    "stationary_processed.csv": "stationary",
    "slow_processed.csv": "slow",
    "vibration_processed.csv": "vibration",
    "tap_processed.csv": "tap"
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
    ])
    writer.writerows(all_features)

print("Feature extraction complete.")
print(f"Saved to {OUTPUT_FILE}")
