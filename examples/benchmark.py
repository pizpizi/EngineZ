import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

df = pd.read_csv('frameRate.csv')
plt.hist(df['frame_time'], bins=50, range=(0, 5000), weights=100*np.ones(len(df))/len(df))
plt.xlabel('Frame Time')
plt.ylabel('Frequency (%)')
plt.show()