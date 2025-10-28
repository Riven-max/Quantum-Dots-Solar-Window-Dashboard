import requests
import random
import time

URL = "http://localhost:3000/api/data/latest"

R = 220  # ohm

while True:
    # Generate random values
    LDR = int(random.uniform(0, 1000))  # Light intensity percentage
    percentage = int((LDR / 1000) * 100)  # Whole number percentage
    voltage = round((percentage / 100) * 12, 3)   # 3 decimal points
    current = round((voltage / R), 3)             # 3 decimal points
    power = round(voltage * current, 3)           # 3 decimal points

    if voltage >= 4.0:
        status = "OK"
    elif voltage >= 2.0:
        status = "LOW"
    else:
        status = "CRITICAL"

    data = {
        "percentage": percentage,
        "voltage": voltage,
        "current": current,
        "power": power,
        "status": status
    }

    try:
        response = requests.post(URL, json=data)
        print(f"Sent: {data}, Response: {response.status_code}")
    except Exception as e:
        print(f"Error sending data: {e}")

    time.sleep(1)
