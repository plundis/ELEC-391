% Clear workspace
clc; clear; close all;

% Simulation parameters
fs = 100;         % Sampling frequency (Hz)
dt = 1/fs;        % Sampling time
T = 10;           % Total simulation time (s)
t = 0:dt:T;       % Time vector

% Create the desired motion profile for accelerometer and gyro angles
gyro_z = zeros(size(t));   % Gyroscope (initializing)
acc_x = zeros(size(t));    % Accelerometer X (initializing)
acc_y = zeros(size(t));    % Accelerometer Y (initializing)

% Angle profiles
acc_angle = zeros(size(t));  % Accelerometer angle (to be updated)
gyro_angle = zeros(size(t)); % Gyroscope angle (to be updated)

% Define the motion segments
for i = 1:length(t)
    if t(i) <= 1
        % Ramp from 0 to 30 degrees in 1 second (same for both)
        target_angle = 30 * t(i);  % Linear ramp to 30 degrees
        gyro_z(i) = 30; % Set angular velocity to achieve ramp
    elseif t(i) <= 6
        % Stay at 30 degrees for 5 seconds (same for both)
        target_angle = 30;
        gyro_z(i) = 0;  % No angular velocity when staying still
    elseif t(i) <= 7
        % Ramp from 30 to 60 degrees quickly (same for both)
        target_angle = 30 + 30 * (t(i) - 6);  % Rapid increase
        gyro_z(i) = 60;  % Set angular velocity for quick ramp
    elseif t(i) <= 8
        % Ramp from 60 to 90 degrees slowly (same for both)
        target_angle = 60 + 30 * (t(i) - 7);  % Slow increase
        gyro_z(i) = 30;  % Set angular velocity for slow increase
    elseif t(i) <= 9
        % Stay at 90 degrees for 1 second (same for both)
        target_angle = 90;
        gyro_z(i) = 0;  % No angular velocity when staying still
    else
        % Ramp back to 0 degrees from 90 degrees very quickly (same for both)
        target_angle = 90 - 90 * (t(i) - 9);  % Rapid decrease
        gyro_z(i) = -90;  % Set angular velocity for rapid decrease
    end
    
    % Update accelerometer and gyroscope angles based on target_angle
    acc_angle(i) = target_angle;          % Accelerometer directly follows target
    if i > 1
        gyro_angle(i) = gyro_angle(i-1) + gyro_z(i) * dt;  % Gyro integration
    end
    
    % Simulate sensor noise on accelerometer (minimal noise)
    acc_x(i) = cosd(target_angle) + 0.01 * randn;  % Simulated accelerometer X
    acc_y(i) = sind(target_angle) + 0.01 * randn;  % Simulated accelerometer Y
end

% Initialize complementary filter
estimated_angle = zeros(size(t));
filtered_angle = zeros(size(t)); % Final filtered output
alpha = 0.6;  % Complementary filter coefficient

% Initial conditions
estimated_angle(1) = acc_angle(1);
filtered_angle(1) = acc_angle(1);

% Complementary Filter Loop
for i = 2:length(t)
    estimated_angle(i) = alpha * gyro_angle(i) + (1 - alpha) * acc_angle(i); % Complementary filter
    filtered_angle(i) = estimated_angle(i); % Store filtered value
end

% Plot results
figure;
plot(t, estimated_angle, 'b', 'LineWidth', 1.5); % Complementary filter output
hold on;
plot(t, acc_angle, 'r--', 'LineWidth', 1.5); % Accelerometer angle
plot(t, gyro_angle, 'g:', 'LineWidth', 1.5); % Integrated gyroscope angle


% Labels and title
xlabel('Time (s)');
ylabel('Angle (deg)');
title('Comparison of Angles (Accelerometer, Gyroscope, Complementary Filter)');
legend('Estimated Angle (Comp. Filter)', 'Accelerometer Angle', ...
       'Gyroscope Angle');
grid on;
hold off;
