import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
import tensorflow as tf

# Load dataset
df = pd.read_csv("sensor_data.csv")

# Extract features and labels
X = df[['Flow1', 'Flow2', 'Pressure']].values
y = df['Leak'].values

# Split into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Normalize the data
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Train logistic regression model
model = LogisticRegression()
model.fit(X_train, y_train)

# Convert to TensorFlow model
input_shape = (3,)
tf_model = tf.keras.Sequential([
    tf.keras.layers.InputLayer(input_shape=input_shape),
    tf.keras.layers.Dense(1, activation='sigmoid')  # Logistic regression in NN form
])

# Set weights from trained logistic regression
weights = [model.coef_.T, np.array(model.intercept_).reshape(-1)]
tf_model.set_weights(weights)


# Convert to TensorFlow Lite
converter = tf.lite.TFLiteConverter.from_keras_model(tf_model)
tflite_model = converter.convert()

# Save the TensorFlow Lite model
with open("leak_detection.tflite", "wb") as f:
    f.write(tflite_model)

print("✅ TFLite model saved as 'leak_detection.tflite'!")
