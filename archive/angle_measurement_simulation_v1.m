% Complementary Filter and PID Controller in MATLAB

clear;
clc;

% Simulation Parameters
dt = 0.01;  % Sampling time (100 Hz)
t = 0:dt:10;  % 10 seconds simulation
n = length(t);

% Sensor Data (Simulated)
gyro_z = 0.5 + 0.05 * randn(1, n);  % Simulated gyroscope readings (rad/s)
acc_x = sin(0.1 * t) + 0.05 * randn(1, n);  % Simulated accelerometer x-axis
acc_y = cos(0.1 * t) + 0.05 * randn(1, n);  % Simulated accelerometer y-axis

% Complementary Filter Parameters
alpha = 0.98;  % Gyro weight (0.98) and accelerometer weight (0.02)
theta = zeros(1, n);  % Estimated angle

% PID Controller Parameters
Kp = 1.5;  % Proportional gain
Ki = 0.1;  % Integral gain
Kd = 0.5;  % Derivative gain
integral = 0;
prev_error = 0;

% Target angle (setpoint)
target_angle = 0;  

% Control output
control_output = zeros(1, n);

for i = 2:n
    % Angle estimation from accelerometer
    acc_angle = atan2(acc_x(i), acc_y(i)) * (180 / pi);
    
    % Angle estimation from gyroscope
    gyro_angle = theta(i-1) + gyro_z(i) * dt;
    
    % Complementary Filter
    theta(i) = alpha * gyro_angle + (1 - alpha) * acc_angle;
    
    % PID Controller
    error = target_angle - theta(i);
    integral = integral + error * dt;
    derivative = (error - prev_error) / dt;
    control_output(i) = Kp * error + Ki * integral + Kd * derivative;
    prev_error = error;
end

% Plot Results
figure;
subplot(3,1,1);
plot(t, theta, 'b', 'LineWidth', 1.5);
hold on;
plot(t, acc_angle, 'r--');
legend('Estimated Angle', 'Accelerometer Angle');
xlabel('Time (s)');
ylabel('Angle (deg)');
title('Complementary Filter Output');

subplot(3,1,2);
plot(t, control_output, 'g', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Control Output');
title('PID Controller Output');

subplot(3,1,3);
plot(t, gyro_z, 'k');
xlabel('Time (s)');
ylabel('Gyro Readings (rad/s)');
title('Gyroscope Data');
