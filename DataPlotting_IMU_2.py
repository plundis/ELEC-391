import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Replace with the correct port and baud rate for your Arduino
port = 'COM3'
baud_rate = 9600

# Open Serial connection
ser = serial.Serial(port, baud_rate)

# Initialize plot with two subplots
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 6), sharex=True)
plt.subplots_adjust(hspace=0.4)  # Add space between subplots

# Data storage
time_data = []
accel_angle_data = []
gyro_angle_data = []

def updatePlot(frame):
    global time_data, accel_angle_data, gyro_angle_data

    # Read a line from Serial port
    arduinoDataString = ser.readline().decode('utf-8').strip()

    try:
        # Extract Accelerometer and Gyroscope angles
        if "Accelerometer angle:" in arduinoDataString and "Gyroscope angle:" in arduinoDataString:
            parts = arduinoDataString.replace("°", "").split(",")
            accel_angle = float(parts[0].split(":")[1].strip())
            gyro_angle = float(parts[1].split(":")[1].strip())

            # Append data
            time_data.append(len(time_data))
            accel_angle_data.append(accel_angle)
            gyro_angle_data.append(gyro_angle)

            # Limit the number of points displayed
            if len(time_data) > 100:
                time_data.pop(0)
                accel_angle_data.pop(0)
                gyro_angle_data.pop(0)

            # Clear and update first subplot (Accelerometer Angle)
            ax1.clear()
            ax1.plot(time_data, accel_angle_data, label="Accelerometer Angle (°)", color='blue')
            ax1.set_title("Accelerometer Angle")
            ax1.set_ylabel("Angle (°)")
            ax1.set_ylim([-180, 180])
            ax1.legend(loc="upper right")
            ax1.grid(True)

            # Clear and update second subplot (Gyroscope Angle)
            ax2.clear()
            ax2.plot(time_data, gyro_angle_data, label="Gyroscope Angle (°)", color='red')
            ax2.set_title("Gyroscope Angle")
            ax2.set_xlabel("Time (samples)")
            ax2.set_ylabel("Angle (°)")
            ax2.set_ylim([-180, 180])
            ax2.legend(loc="upper right")
            ax2.grid(True)

    except (ValueError, IndexError):
        pass  # Ignore invalid or incomplete lines

# Animate the plots
ani = animation.FuncAnimation(fig, updatePlot, frames=100, interval=100)

# Show the plot
plt.show()

# Close the Serial connection when done
ser.close()
