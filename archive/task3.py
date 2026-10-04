import serial
import matplotlib.pyplot as plt
import numpy as np

# Set up serial connection
port = 'COM5'  # Update this to the correct port for your Arduino
baud_rate = 9600
ser = serial.Serial(port, baud_rate, timeout=1)

# Prepare lists to store accelerometer angle data
acc_angles = []
time = []

# Set up the plot
plt.ion()
fig, ax = plt.subplots()

# Set plot labels and title
ax.set_xlabel('Time (s)')
ax.set_ylabel('Angle (°)')
ax.set_title('Gyroscope Angle')

# Initialize time counter
start_time = np.datetime64('now')

# Plot line for accelerometer angle
line_acc, = ax.plot([], [], label='Gyroscope Angle', color='g')

# Add legend to the plot (initially with placeholders)
legend = ax.legend()

# Continuously read data from the Arduino and plot
iteration = 0  # To control the update frequency
while True:
    try:
        # Read data from Arduino (expects the format as mentioned)
        line = ser.readline().decode('utf-8').strip()
        
        if line.startswith("Gyroscope angle:"):
            # Extract the accelerometer angle from the data string
            acc_ang = float(line.split(":")[1].strip().replace("°", ""))
            
            # Append the data to the list
            acc_angles.append(acc_ang)
            
            # Get the current time and convert to seconds
            current_time = np.datetime64('now')
            elapsed_time = (current_time - start_time) / np.timedelta64(1, 's')
            time.append(elapsed_time)
            
            # Update the plot at a controlled frequency (e.g., every 10 iterations)
            if iteration % 10 == 0:
                line_acc.set_data(time, acc_angles)
                
                # Update the legend with the current accelerometer angle value
                legend_text = [f'Gyroscope Angle: {acc_ang:.2f}°']
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
