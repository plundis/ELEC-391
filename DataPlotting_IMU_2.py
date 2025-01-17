import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation

port = 'COM3'  # Replace with the correct port
baud_rate = 9600

# Open Serial connection
ser = serial.Serial(port, baud_rate)

# Initialize plot with three subplots
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(8, 8), sharex=True)
plt.subplots_adjust(hspace=0.5)

# Data storage
time_data = []  # Continuous time axis
filtered_angle_data = []
accel_angle_data = []
gyro_angle_data = []

max_points = 50  # Maximum number of points to display

def updatePlot(frame):
    global time_data, filtered_angle_data, accel_angle_data, gyro_angle_data

    # Read a line from Serial port
    arduinoDataString = ser.readline().decode('utf-8').strip()
    print(f"Received: {arduinoDataString}")  # Debug: Display received data

    try:
        # Parse the serial data
        if "Filtered angle:" in arduinoDataString and "Accelerometer angle:" in arduinoDataString and "Gyroscope angle:" in arduinoDataString:
            parts = arduinoDataString.replace("°", "").split(",")
            filtered_angle = float(parts[0].split(":")[1].strip())
            accel_angle = float(parts[1].split(":")[1].strip())
            gyro_angle = float(parts[2].split(":")[1].strip())

            # Update time and angle data
            if time_data:
                time_data.append(time_data[-1] + 1)  # Increment time by 1
            else:
                time_data.append(0)  # Initialize time data

            filtered_angle_data.append(filtered_angle)
            accel_angle_data.append(accel_angle)
            gyro_angle_data.append(gyro_angle)

            # Limit the number of points displayed for angles
            filtered_angle_data = filtered_angle_data[-max_points:]
            accel_angle_data = accel_angle_data[-max_points:]
            gyro_angle_data = gyro_angle_data[-max_points:]

            # Limit time_data to match the length of angle data
            time_data = time_data[-max_points:]

            # Clear and update first subplot (Filtered Angle)
            ax1.clear()
            ax1.plot(time_data, filtered_angle_data, label="Filtered Angle (°)", color='green')
            ax1.set_title("Filtered Angle")
            ax1.set_ylabel("Angle (°)")
            ax1.set_ylim([-180, 180])
            ax1.legend(loc="upper right")
            ax1.grid(True)

            # Clear and update second subplot (Accelerometer Angle)
            ax2.clear()
            ax2.plot(time_data, accel_angle_data, label="Accelerometer Angle (°)", color='blue')
            ax2.set_title("Accelerometer Angle")
            ax2.set_ylabel("Angle (°)")
            ax2.set_ylim([-180, 180])
            ax2.legend(loc="upper right")
            ax2.grid(True)

            # Clear and update third subplot (Gyroscope Angle)
            ax3.clear()
            ax3.plot(time_data, gyro_angle_data, label="Gyroscope Angle (°)", color='red')
            ax3.set_title("Gyroscope Angle")
            ax3.set_xlabel("Time (samples)")
            ax3.set_ylabel("Angle (°)")
            ax3.set_ylim([-180, 180])
            ax3.legend(loc="upper right")
            ax3.grid(True)

    except (ValueError, IndexError):
        print(f"Error: Could not parse line: {arduinoDataString}")

# Animate the plots
ani = animation.FuncAnimation(fig, updatePlot, frames=100, interval=100)

# Show the plot
plt.show()

# Close the Serial connection when done
ser.close()
