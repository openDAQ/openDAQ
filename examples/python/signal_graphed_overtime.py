import opendaq
import time
import numpy as np
import matplotlib.pyplot as plt


def select_device(instance):
    available_devices_info = instance.available_devices
    print("Available devices - Select one to connect to:")
    for i, device_info in enumerate(available_devices_info):
        print(f"{i}: Name: {device_info.name}, Connection string: {device_info.connection_string}")

    device_index = int(input("Enter the index of the device you want to connect to: "))
    if 0 <= device_index < len(available_devices_info):
        return instance.add_device(available_devices_info[device_index].connection_string)
    else:
        print("Invalid index")
        exit()


def read_signal(device):
    signals = device.signals_recursive
    print("Signals:")
    for i, signal in enumerate(signals):
        print(f"{i}: {signal.name}")

    signal_index = int(input("Enter the index of the signal you want to read: "))
    if 0 <= signal_index < len(signals):
        return signals[signal_index]
    else:
        print("Invalid index")
        exit()


def plot_signal_values(signal_values, signal_name):
    time_axis = np.linspace(0, 10, len(signal_values))
    plt.plot(time_axis, signal_values)
    plt.xlabel('Time (s)')
    plt.ylabel('Signal value')
    plt.title(f'Signal values over 10 seconds: {signal_name}')
    plt.show()


def calculate_statistics(signal_values):
    avg = np.mean(signal_values)
    min_val = np.min(signal_values)
    max_val = np.max(signal_values)
    rms = np.sqrt(np.mean(signal_values ** 2))
    print(f'Average value: {avg}\nMaximum value: {max_val}\nMinimum value: {min_val}\nMean root square value: {rms}')


def main():
    instance = opendaq.Instance()
    device = select_device(instance)
    print(f"\nConnected to device: {device.info.name}")

    signal = read_signal(device)
    print(f"Reading signal: {signal.name}\n")

    reader = opendaq.StreamReader(signal,timeout_type=opendaq.ReadTimeoutType.All)
    signal_values = [reader.read(100, 100) for _ in range(int(10 / 0.1))]
    signal_values = np.concatenate(signal_values)

    plot_signal_values(signal_values, signal.name)
    calculate_statistics(signal_values)


if __name__ == "__main__":
    main()