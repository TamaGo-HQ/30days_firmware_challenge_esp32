import pandas as pd
import matplotlib.pyplot as plt

# Load feature file
df = pd.read_csv("features.csv")

# List of features to inspect
features = [
    "acc_rms",
    "acc_var",
    "acc_peak",
    "gyro_rms",
    "gyro_var",
    "gyro_peak"
]

# ----------------------------
# Boxplots per feature
# ----------------------------
for feature in features:
    fig, ax = plt.subplots()   # create exactly ONE figure

    df.boxplot(column=feature, by="label", ax=ax)

    ax.set_title(f"{feature} by motion class")
    ax.set_ylabel(feature)

    plt.suptitle("")   # remove automatic pandas title
    plt.tight_layout()
    plt.show()