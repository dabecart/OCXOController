import serial
import csv
import time
import re

# Serial port settings
SERIAL_PORT = "COM6"  # Change this according to your system (e.g., "/dev/ttyUSB0" for Linux)
BAUD_RATE = 921600
CSV_FILE = "data.csv"

# Regular expression pattern to extract decimal numbers
DATA_PATTERN = re.compile(r"D:\s*(-?\d+\.\d+)")

def setup_csv():
    """Ensures that the CSV file has a header if it does not exist."""
    try:
        with open(CSV_FILE, mode='r') as file:
            pass  # File exists, do nothing
    except FileNotFoundError:
        with open(CSV_FILE, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(["Timestamp", "Value"])  # Write header

def read_serial():
    """Reads data from the serial port and logs it to a CSV file."""
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser, open(CSV_FILE, mode='a', newline='') as file:
            writer = csv.writer(file)

            while True:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                match = DATA_PATTERN.match(line)

                if match:
                    value = float(match.group(1))
                    formatted_value = str(value).replace(".", ",")
                    
                    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")  # Human-readable timestamp
                    writer.writerow([timestamp, value])
                    file.flush()  # Ensure data is written immediately
                    print(f"Logged: {timestamp}; {formatted_value}")

    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nProgram terminated by user.")
    except Exception as e:
        print(f"Unexpected error: {e}")

if __name__ == "__main__":
    setup_csv()
    print("Listening to serial port...")
    read_serial()
