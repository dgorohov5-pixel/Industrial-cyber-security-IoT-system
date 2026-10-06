import serial
import time
from datetime import datetime

# Serial Port Configuration
COM_PORT = 'COM10'  
BAUD_RATE = 9600

# Dictionary to track running emergencies: {incident_type: start_time}
active_emergencies = {}

def parse_incident_type(raw_line):
    """
    Extracts the core threat from the Arduino log string.
    """
    line_upper = raw_line.upper()
    if "FIRE" in line_upper or "FLAME" in line_upper:
        return "Fire Hazard"
    elif "GAS" in line_upper:
        return "Gas Leak"
    elif "DRONE" in line_upper:
        return "Drone Detected"
    elif "INTRUSION" in line_upper:
        return "Intrusion"
    return "Unknown"

def process_telemetry_line(line):
    """
    Parses incoming Arduino tags and writes timestamps to log file.
    """
    now = datetime.now()
    time_str = now.strftime("%Y-%m-%d %H:%M:%S")
    unix_now = time.time() 

    # Handle incoming Alert triggers
    if "[ALERT]" in line:
        incident = parse_incident_type(line)
        if incident not in active_emergencies:
            active_emergencies[incident] = unix_now
            log_entry = f"[{time_str}] [CRITICAL] {incident} STARTED\n"
            write_to_file(log_entry)
            print(f"[*] Started logging for: {incident}")

    # Handle successful hazard resolution
    elif "[OK]" in line:
        incident = parse_incident_type(line)
        if incident in active_emergencies:
            start_time = active_emergencies.pop(incident)
            duration = int(unix_now - start_time)
            log_entry = f"[{time_str}] [RESOLVED] {incident} FIXED. Duration: {duration} seconds.\n"
            write_to_file(log_entry)
            print(f"[+] Closed log for: {incident}. Duration: {duration}s")

def write_to_file(text):
    """
    Appends logs to the text file.
    """
    try:
        # The way for my computer
        with open(r"C:\Users\Администратор\Desktop\рабстол_files\security_log.txt", "a", encoding="utf-8") as file:
            file.write(text)
    except FileNotFoundError:
        # The way for any other computers
        with open("security_log.txt", "a", encoding="utf-8") as file:
            file.write(text)

def start_logger():
    print(f"[INIT] Connecting to Arduino on {COM_PORT}...")
    try:
        # Open serial port connection
        arduino_serial = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # Wait for Arduino connection delay
        print("[SUCCESS] Connected. Waiting for data...\n")
        
        while True:
            if arduino_serial.in_waiting > 0:
                raw_data = arduino_serial.readline().decode('utf-8').strip()
                if raw_data:
                    print(f"[Data] {raw_data}")
                    process_telemetry_line(raw_data)
                    
    except serial.SerialException:
        print(f"[ERROR] Cannot open port. Check connection.")
    except KeyboardInterrupt:
        print("\n[STOP] Script stopped by user.")

if __name__ == "__main__":
    start_logger()

