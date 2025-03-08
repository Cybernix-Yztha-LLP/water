import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression

# Load the sensor data again
sensor_data_path = "/sensor_data.csv"
df = pd.read_csv(sensor_data_path)

# Separate features and target variable
X = df[['Flow1', 'Flow2', 'Pressure']].values
y = df['Leak'].values

# Split into training and test sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the features
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# Train logistic regression model
log_reg = LogisticRegression()
log_reg.fit(X_train_scaled, y_train)

# Extract model weights and bias
weights = log_reg.coef_[0]  # Coefficients
bias = log_reg.intercept_[0]  # Bias term
scaler_means = scaler.mean_  # Mean for scaling
scaler_stds = scaler.scale_  # Std deviation for scaling

# Generate C++ header file content
cpp_code = f"""#ifndef CUSTOM_MODEL_H
#define CUSTOM_MODEL_H

#include <cmath>

// Logistic Regression Model for Leak Detection
class LeakDetectionModel {{
public:
    // Standardization parameters (mean and std deviation)
    static constexpr float means[3] = {{{scaler_means[0]:.6f}, {scaler_means[1]:.6f}, {scaler_means[2]:.6f}}};
    static constexpr float stds[3] = {{{scaler_stds[0]:.6f}, {scaler_stds[1]:.6f}, {scaler_stds[2]:.6f}}};

    // Logistic regression weights and bias
    static constexpr float weights[3] = {{{weights[0]:.6f}, {weights[1]:.6f}, {weights[2]:.6f}}};
    static constexpr float bias = {bias:.6f};

    // Sigmoid function
    static float sigmoid(float x) {{
        return 1.0f / (1.0f + exp(-x));
    }}

    // Predict leak probability
    static float predict(float flow1, float flow2, float pressure) {{
        // Standardize inputs
        float norm_flow1 = (flow1 - means[0]) / stds[0];
        float norm_flow2 = (flow2 - means[1]) / stds[1];
        float norm_pressure = (pressure - means[2]) / stds[2];

        // Compute linear combination (dot product)
        float linear_output = norm_flow1 * weights[0] + norm_flow2 * weights[1] + norm_pressure * weights[2] + bias;

        // Apply sigmoid activation
        return sigmoid(linear_output);
    }}
}};

#endif // CUSTOM_MODEL_H
"""

# Save the generated C++ header file
custom_model_path = "custom_model.h"
with open(custom_model_path, "w") as f:
    f.write(cpp_code)

custom_model_path
