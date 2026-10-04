import serial
import matplotlib.pyplot as plt
import numpy as np

# Set up serial connection
port = 'COM5'  # Update this to the correct port for your Arduino
baud_rate = 9600
ser = serial.Serial(port, baud_rate, timeout=1)

# Prepare lists to store angle data
filtered_angles = []
acc_angles = []
gyro_angles = []
time = []

# Set up the plot
plt.ion()
fig, ax = plt.subplots()

# Set plot labels and title
ax.set_xlabel('Time (s)')
ax.set_ylabel('Angle (°)')
ax.set_title('Filtered, Accelerometer, and Gyroscope Angles')

# Initialize time counter
start_time = np.datetime64('now')

# Plot lines for filtered, accelerometer, and gyroscope angles
line_filtered, = ax.plot([], [], label='Filtered Angle', color='b')
line_acc, = ax.plot([], [], label='Accelerometer Angle', color='g')
line_gyro, = ax.plot([], [], label='Gyroscope Angle', color='r')

# Add legend to the plot (initially with placeholders)
legend = ax.legend()

# Continuously read data from the Arduino and plot
iteration = 0  # To control the update frequency
while True:
    try:
        # Read data from Arduino (expects the format as mentioned)
        line = ser.readline().decode('utf-8').strip()
        
        if line.startswith("Filtered angle:"):
            # Extract angles from the data string
            parts = line.split(',')
            filtered_ang = float(parts[0].split(":")[1].strip().replace("°", ""))
            acc_ang = float(parts[1].split(":")[1].strip().replace("°", ""))
            gyro_ang = float(parts[2].split(":")[1].strip().replace("°", ""))
            
            # Append the data to the lists
            filtered_angles.append(filtered_ang)
            acc_angles.append(acc_ang)
            gyro_angles.append(gyro_ang)
            
            # Get the current time and convert to seconds
            current_time = np.datetime64('now')
            elapsed_time = (current_time - start_time) / np.timedelta64(1, 's')
            time.append(elapsed_time)
            
            # Update the plot at a controlled frequency (e.g., every 10 iterations)
            if iteration % 10 == 0:
                line_filtered.set_data(time, filtered_angles)
                line_acc.set_data(time, acc_angles)
                line_gyro.set_data(time, gyro_angles)
                
                # Update the legend with the current angle values
                legend_text = [f'Filtered Angle: {filtered_ang:.2f}°', 
                               f'Accelerometer Angle: {acc_ang:.2f}°', 
                               f'Gyroscope Angle: {gyro_ang:.2f}°']
                for i, text in enumerate(legend.get_texts()):
                    text.set_text(legend_text[i])
                
                # Redraw only the updated data
                ax.relim()  # Recalculate limits
                ax.autoscale_view()  # Rescale the view
                plt.pause(0.01)  # Small pause to allow the plot to update

            iteration += 1

    except KeyboardInterrupt:
        break

# Close the serial connection
ser.close()
