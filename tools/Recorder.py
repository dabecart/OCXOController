import serial
import csv
import time
import re

# Serial port settings
SERIAL_PORT = "COM6"  # Change this according to your system (e.g., "/dev/ttyACM0" for Linux)
BAUD_RATE = 921600
CSV_FILE = "data" + time.strftime("%Y-%m-%d_%H-%M-%S") + ".csv"

FREQ_PATTERN  = re.compile(r"F=\s*(-?\d+\.\d+)")
VCO_PATTERN   = re.compile(r"VCO=\s*(-?\d+\.\d+),\s*(-?\d+)")
ERROR_PATTERN = re.compile(r"e=\s*(-?\d+\.\d+), Kp=\s*(-?\d+\.\d+)")
INTG_PATTERN  = re.compile(r"i=\s*(-?\d+\.\d+), Ki=\s*(-?\d+\.\d+)")
DERV_PATTERN  = re.compile(r"d=\s*(-?\d+\.\d+), Kd=\s*(-?\d+\.\d+)")
OFFS_PATTERN  = re.compile(r"Of=\s*(-?\d+\.\d+)")

def setup_csv():
    # Ensures that the CSV file has a header if it does not exist.
    try:
        with open(CSV_FILE, mode='r') as file:
            pass  # File exists, do nothing
    except FileNotFoundError:
        with open(CSV_FILE, mode='w', newline='') as file:
            writer = csv.writer(file, delimiter=';')
            # Write header
            writer.writerow(["Time", "OCXO Freq", "VCO", "VCO int", "e", "Kp", "i", "Ki", "d", "Kd", "Offset"])

def read_serial():
    # Reads data from the serial port and logs it to a CSV file.
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser, open(CSV_FILE, mode='a', newline='') as file:
            ser.write(b'CONN\n')

            writer = csv.writer(file, delimiter=';')
            valueGroup = [""] * 10

            while True:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                
                match = FREQ_PATTERN.match(line)
                if match:
                    # Save the previous group.
                    if valueGroup[0] != "":
                        # Human-readable timestamp
                        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")  

                        writer.writerow([timestamp, *valueGroup])
                        # Ensure data is written immediately
                        file.flush()  
                        print(f"Logged: {timestamp}; {valueGroup}")
                        
                        # Leave it empty.
                        valueGroup = [""] * 10

                    valueGroup[0] = str(float(match.group(1))).replace(".", ",")

                match = VCO_PATTERN.match(line)
                if match:
                    valueGroup[1] = str(float(match.group(1))).replace(".", ",")
                    valueGroup[2] = str(int(match.group(2)))

                match = ERROR_PATTERN.match(line)
                if match:
                    valueGroup[3] = str(float(match.group(1))).replace(".", ",")
                    valueGroup[4] = str(float(match.group(2))).replace(".", ",")

                match = INTG_PATTERN.match(line)
                if match:
                    valueGroup[5] = str(float(match.group(1))).replace(".", ",")
                    valueGroup[6] = str(float(match.group(2))).replace(".", ",")

                match = DERV_PATTERN.match(line)
                if match:
                    valueGroup[7] = str(float(match.group(1))).replace(".", ",")
                    valueGroup[8] = str(float(match.group(2))).replace(".", ",")

                match = OFFS_PATTERN.match(line)
                if match:
                    valueGroup[9] = str(float(match.group(1))).replace(".", ",")

                
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
