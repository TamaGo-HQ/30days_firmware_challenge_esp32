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
        "vibration.csv": "Vibration",
        "tap.csv": "Impact",
    }

    for filename, title in files.items():
        plot_motion(filename, title)
