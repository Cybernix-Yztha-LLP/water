import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
import tensorflow as tf

# Load dataset
df = pd.read_csv("sensor_data.csv")

# Separate features and target variable
X = df[['Flow1', 'Flow2', 'Pressure']]
y = df['Leak']

# Split into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize features
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# Train logistic regression model
log_reg = LogisticRegression()
log_reg.fit(X_train_scaled, y_train)

# Convert to TensorFlow model
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(3,)),
    tf.keras.layers.Dense(1, activation='sigmoid')
])
model.compile(optimizer='adam', loss='binary_crossentropy', metrics=['accuracy'])

# Set weights from trained logistic regression model
weights = [log_reg.coef_.T, log_reg.intercept_]
model.layers[0].set_weights(weights)

# Convert to TensorFlow Lite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# Save the model
with open("logistic_regression_model.tflite", "wb") as f:
    f.write(tflite_model)

print("Model converted and saved as logistic_regression_model.tflite")
