import time
import bluetooth
import machine
import esp32
from machine import UART, Pin, I2C, ADC
import ssd1306

# ------------------------------------------------------------
# PIN LAYOUT (Heltec V3 Hardware Mapping)
# ------------------------------------------------------------
GPS_RX, GPS_TX = 4, 5
FLARM_RX, FLARM_TX = 6, 7  
LED_PIN = Pin(2, Pin.OUT)

OLED_SDA, OLED_SCL = 17, 18
OLED_RST, OLED_PWR = 21, 36

BAT_PIN = Pin(1) 
ADC_CTRL = Pin(37, Pin.OUT, value=1) 

PRG_BUTTON = Pin(0, Pin.IN)

# ------------------------------------------------------------
# 1. HARDWARE INITIALIZATION
# ------------------------------------------------------------
pin_pwr = Pin(OLED_PWR, Pin.OUT, value=0) 
pin_rst = Pin(OLED_RST, Pin.OUT, value=1)
time.sleep_ms(150)

adc = ADC(BAT_PIN)
adc.atten(ADC.ATTN_11DB) 

i2c = I2C(0, sda=Pin(OLED_SDA), scl=Pin(OLED_SCL), freq=400000)
oled = None
try:
    oled = ssd1306.SSD1306_I2C(128, 64, i2c, addr=60)
except: 
    print("OLED Init failed, bypassing display...")

def get_battery_visual(pct):
    filled = int(pct / 12.5) 
    return "[" + "|" * filled + " " * (8 - filled) + "]"

def update_screen(status, pilots, bat_pct):
    if not oled: 
        return
    try:
        oled.fill(0)
        oled.text("AirWhere S3", 0, 0)
        oled.text(f"{get_battery_visual(bat_pct)} {bat_pct}%", 0, 10)
        oled.text("----------------", 0, 20)
        
        oled.text(status, 0, 32)
        oled.text("Name: AW-S3", 0, 42)
        
        oled.text(f"FLARM count: {pilots}", 0, 56) 
        oled.show()
    except Exception as e:
        print("Screen draw error:", e)

def power_down_device():
    print("Executing system shutdown sequence...")
    if oled:
        try:
            oled.fill(0)
            oled.text("SHUTTING DOWN...", 0, 24)
            oled.show()
            time.sleep_ms(1000)
            oled.poweroff()
        except: pass
    LED_PIN.value(0)
    ADC_CTRL.value(0)
    pin_pwr.value(1) # Sever peripheral power rail
    esp32.wake_on_ext0(pin=PRG_BUTTON, level=esp32.WAKEUP_ALL_LOW)
    machine.deepsleep()

# ------------------------------------------------------------
# 2. BLE WIRELESS ENGINE (Watchdog Deferral Architecture)
# ------------------------------------------------------------
class BLEAirWhere:
    def __init__(self, name="AW-S3"):
        self.ble = bluetooth.BLE()
        self.ble.active(True)
        self.ble.irq(self._irq)
        self.connections = set()
        self.ble_should_advertise = False
        
        HM10_SRV = bluetooth.UUID(0xFFE0)
        HM10_CHR = bluetooth.UUID(0xFFE1)
        services = ((HM10_SRV, ((HM10_CHR, bluetooth.FLAG_NOTIFY | bluetooth.FLAG_WRITE | bluetooth.FLAG_WRITE_NO_RESPONSE),),),)
        (self.handle,) = self.ble.gatts_register_services(services)
        
        self.payload = b'\x06\x01\x06\x03\x03\xe0\xff\x09\x09AW-S3   '
        self.start_advertising()

    def _irq(self, event, data):
        if event == 1: 
            conn_handle, _, _ = data
            self.connections.add(conn_handle)
            LED_PIN.value(1)
        elif event == 2: 
            conn_handle, _, _ = data
            if conn_handle in self.connections: 
                self.connections.remove(conn_handle)
            LED_PIN.value(0)
            self.ble_should_advertise = True

    def is_connected(self):
        return len(self.connections) > 0

    def start_advertising(self):
        try: self.ble.gap_advertise(None)
        except: pass
        try:
            self.ble.gap_advertise(200000, adv_data=self.payload)
            self.ble_should_advertise = False
        except: pass

    def send_telemetry(self, data):
        for conn_handle in self.connections:
            try: self.ble.gatts_notify(conn_handle, self.handle, data)
            except: pass

# ------------------------------------------------------------
# 3. MAIN RUNTIME LOOP
# ------------------------------------------------------------
gps_serial = UART(1, baudrate=9600, rx=GPS_RX, tx=GPS_TX, timeout=10)
flarm_serial = UART(2, baudrate=57600, rx=FLARM_RX, tx=FLARM_TX, timeout=10)

airwhere = BLEAirWhere("AW-S3")
active_targets = 0
bat_pct = 100 

last_ui_update = time.ticks_ms()
button_hold_start = None

print("AirWhere S3 Flight Core Engine Operational...")
update_screen("STATUS: SEARCHING", active_targets, bat_pct)

while True:
    time.sleep_ms(20) 
    
    if PRG_BUTTON.value() == 0:  
        if button_hold_start is None:
            button_hold_start = time.ticks_ms()
        elif time.ticks_diff(time.ticks_ms(), button_hold_start) > 2000:
            power_down_device()  
    else:
        button_hold_start = None 
        
    if airwhere.ble_should_advertise:
        airwhere.start_advertising()
    
    if time.ticks_diff(time.ticks_ms(), last_ui_update) > 3000:
        try:
            raw = adc.read_u16()
            voltage = (raw / 65535) * 3.3 * 4.9                  
            bat_pct = int(((voltage - 3.3) / (4.2 - 3.3)) * 100) 
            bat_pct = max(0, min(100, bat_pct))
        except: pass
            
        status = "STATUS: PAIRED" if airwhere.is_connected() else "STATUS: SEARCHING"
        update_screen(status, active_targets, bat_pct)
        last_ui_update = time.ticks_ms()

    if gps_serial.any():
        line_data = gps_serial.readline()
        if line_data and airwhere.is_connected():
            airwhere.send_telemetry(line_data)

    if flarm_serial.any():
        line_data = flarm_serial.readline()
        if line_data:
            if b"$PFLAU," in line_data:
                try:
                    sentence = line_data.decode('ascii', 'ignore').strip()
                    parts = sentence.split(',')
                    if len(parts) > 1 and parts[1].isdigit():
                        active_targets = int(parts[1])
                except: pass
                    
            if airwhere.is_connected():
                airwhere.send_telemetry(line_data)