import numpy as np

# Read .tflite model file
with open("leak_detection.tflite", "rb") as f:
    tflite_model = f.read()

# Convert to a C array
c_array = np.array(list(tflite_model), dtype=np.uint8)

# Save to a header file
with open("model_data.h", "w") as f:
    f.write("#ifndef MODEL_DATA_H\n#define MODEL_DATA_H\n\n")
    f.write("#include <stdint.h>\n\n")  # Include standard types
    f.write(f"alignas(8) const unsigned char model_data[] = {{\n")  # Alignment for embedded systems

    # Break long arrays into multiple lines
    for i in range(0, len(c_array), 12):  # 12 bytes per line (adjust as needed)
        f.write("    " + ", ".join(map(str, c_array[i:i+12])) + ",\n")

    f.write("};\n\n")
    f.write(f"const unsigned int model_data_len = {len(c_array)};\n\n")
    f.write("#endif // MODEL_DATA_H\n")

print("✅ Header file generated: model_data.h")
