#ifndef CUSTOM_MODEL_H
#define CUSTOM_MODEL_H

#include <cmath>

// Logistic Regression Model for Leak Detection
class LeakDetectionModel {
public:
    // Standardization parameters (mean and std deviation)
    static constexpr float means[3] = {0.649487, 0.278802, 3.239087};
    static constexpr float stds[3] = {0.398183, 0.376616, 0.086358};

    // Logistic regression weights and bias
    static constexpr float weights[3] = {3.279568, -4.543833, -0.010264};
    static constexpr float bias = 0.843646;

    // Sigmoid function
    static float sigmoid(float x) {
        return 1.0f / (1.0f + exp(-x));
    }

    // Predict leak probability
    static float predict(float flow1, float flow2, float pressure) {
        // Standardize inputs
        float norm_flow1 = (flow1 - means[0]) / stds[0];
        float norm_flow2 = (flow2 - means[1]) / stds[1];
        float norm_pressure = (pressure - means[2]) / stds[2];

        // Compute linear combination (dot product)
        float linear_output = norm_flow1 * weights[0] + norm_flow2 * weights[1] + norm_pressure * weights[2] + bias;

        // Apply sigmoid activation
        return sigmoid(linear_output);
    }
};

#endif // CUSTOM_MODEL_H
