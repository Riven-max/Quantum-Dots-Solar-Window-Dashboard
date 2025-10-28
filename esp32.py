import network
import urequests
import utime
from machine import Pin, ADC, I2C
from i2c_lcd import I2cLcd

#WiFi Credentials
WIFI_SSID = "PARADOX"
WIFI_PASS = "shouryakumar"

#Backend URL
BACKEND_URL = "http://192.168.1.100:3000/api/data/latest"

#Setup Sensors
LDR = ADC(Pin(34))  # LDR sensor

#LCD Setup
i2c = I2C(0, scl=Pin(22), sda=Pin(21), freq=400000)

#LCD address
lcd = I2cLcd(i2c, 0x27, 2, 16)  # (i2c, address, rows, columns)

#Display Text
lcd.clear()
lcd.putstr("MASTERS©")

#Load Control (Bulb/Motor)
load = Pin(26, Pin.OUT)  # GPIO26 → MOSFET Gate or Relay IN pin
load.on()  # Always turn ON load

#WiFi Connect
def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PASS)
    print("Connecting to WiFi...", end="")
    while not wlan.isconnected():
        utime.sleep(1)
        print(".", end="")
    print("\nConnected! IP:", wlan.ipconfig()[0])

#Main Loop
def send_data():
    while True:
        # Simulated readings (scale ADC)
        LDR = LDR.read()
        percentage = int((LDR.read / 1000) * 100)
        voltage = round((percentage / 100) * 12, 3)
        current = round((voltage / 220), 3)  # Assuming Resistance = 220 ohm
        power = round(voltage * current, 3)

        # Status (info only, does not control load now)
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
            res = urequests.post(BACKEND_URL, json=data)
            print("Data sent:", data, "| Response:", res.text)
            res.close()
        except Exception as e:
            print("Error sending data:", e)

        utime.sleep(1)

#Run
connect_wifi()
send_data()