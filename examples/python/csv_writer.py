import numpy as np
import opendaq
import time
import argparse
import threading


class Recorder:
    def __init__(self, signal, buffer_size):
        self.signal_descriptor = signal.descriptor
        self.domain_signal_descriptor = signal.domain_signal.descriptor
        self.buffer_size = buffer_size
        self.read = 0
        self.reader = opendaq.TimeStreamReader(opendaq.StreamReader(signal))
        self.values_buffer = []
        self.domain_buffer = []

    def read_samples(self):
        self.read = len(self.values_buffer) or self.buffer_size
        values, domain = self.reader.read_with_timestamps(self.read)
        self.read = len(values)
        self.values_buffer = values
        self.domain_buffer = domain

    def clear_buffer(self):
        self.read = 0

    def print_samples(self, max_to_print):
        print(f"{self.signal_descriptor.name}: ", end='')
        for i in range(min(self.read, max_to_print)):
            print(f"{self.values_buffer[i]} ", end='')
        print('')


class RecordedDevice:
    first_write = True

    def __init__(self, name, recorders):
        self.recorders = recorders
        self.filename = name.replace(':', '_') + '.csv'
        self.csv_file = open(self.filename, 'w')

    def write_header(self):
        for recorder in self.recorders:
            self.csv_file.write(f"{recorder.signal_descriptor.name};;")
        self.csv_file.write('\n')
        self.csv_file.flush()

    def write_buffers(self):
        for i in range(max(r.read for r in self.recorders)):
            for recorder in self.recorders:
                self.write_sample(recorder, i)
            self.csv_file.write('\n')

        for recorder in self.recorders:
            recorder.clear_buffer()

    def write_sample(self, recorder, index):
        if index < recorder.read:
            if self.first_write:
                tdata = np.datetime_as_string(recorder.domain_buffer[index])
                self.csv_file.write(f"{tdata};\n")
                self.first_write = False

            tdata = np.datetime_as_string(recorder.domain_buffer[index])
            self.csv_file.write(
                f"{tdata};{recorder.values_buffer[index]:.2f};")
            self.first_write = False
        else:
            self.csv_file.write(';')

    def print_samples(self, channel_name, buffer, max_to_print):
        print(f"{channel_name}: ", end='')
        for i in range(min(self.read, max_to_print)):
            print(f"{buffer[i]} ", end='')
        print('')

    def __del__(self):
        self.csv_file.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Csv Writer', epilog='Example: csv_writer.py --wsport 7414 --connection daqref://device1')
    parser.add_argument('--wsport', help='websocket port', type=int)
    parser.add_argument(
        '--connection', help='connection string', default='any')
    args = parser.parse_args()

    connection_string = args.connection

    instance = opendaq.Instance()
    server_types = instance.available_server_types.values()

    device_info_list = instance.available_devices
    devices = []

    print('Discovered devices:')

    for device_info in device_info_list:
        print('Name:', device_info.name, 'Connection string:',
              device_info.connection_string)

    def valid(d):
        return connection_string == 'any' or d.connection_string == connection_string
    devices = [instance.add_device(d.connection_string)
               for d in device_info_list if valid(d)]

    # create readers and writers
    recorded_devices = []
    for i, device in enumerate(devices):
        recorders = []
        device_name = f'device{i}_{device.info.name}'

        signals = device.signals_recursive

        print(device_name)

        for signal in signals:

            if signal.domain_signal:
                recorders.append(Recorder(signal, 1000))
                print(f"{signal.descriptor.name}")
                print(f"{signal.global_id}")
        recorded_device = RecordedDevice(device_name, recorders)
        recorded_device.write_header()
        recorded_devices.append(recorded_device)

    stop = False

    def device_loop():
        global stop
        while not stop:
            for recorded_device in recorded_devices:
                for recorder in recorded_device.recorders:
                    recorder.read_samples()
                recorded_device.write_buffers()
            time.sleep(0.02)

    t = threading.Thread(target=device_loop)
    t.start()

    print("Recording, type 'stop' to stop...")

    while not stop:
        stop = input() == 'stop'

    t.join()
