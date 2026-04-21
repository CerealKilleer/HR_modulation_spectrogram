import struct
import numpy as np
import neurokit2 as nk
import paho.mqtt.client as mqtt

BROKER = "192.168.88.196"
PORT = 8883
USER = "yonathan"
PASS = "Celeste246-"
CAFILE = "/etc/mosquitto/certs/ca.crt"
FS = 256
BUFFER_SECONDS = 4
BUFFER_SIZE = FS * BUFFER_SECONDS

buffer = []

def calcular_bpm(samples):
    signal = np.array(samples, dtype=np.float32)
    signal = (signal / 4095.0) * 3.3

    try:
        signals, info = nk.ecg_process(signal, sampling_rate=FS)
        peaks = info["ECG_R_Peaks"]
        if len(peaks) < 2:
            return None
        intervals = np.diff(peaks) / FS
        bpm = int(round(60.0 / np.mean(intervals)))
        if 30 <= bpm <= 220:
            return bpm
    except Exception as e:
        print(f"Error procesando ECG: {e}")

    return None

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Conectado al broker")
        client.subscribe("/ECG")
    else:
        print(f"Error de conexión: {rc}")

def on_message(client, userdata, msg):
    global buffer

    if len(msg.payload) < 64:
        return

    samples = list(struct.unpack('<32H', msg.payload[:64]))
    buffer.extend(samples)

    if len(buffer) >= BUFFER_SIZE:
        bpm = calcular_bpm(buffer[-BUFFER_SIZE:])
        buffer = list()
        if bpm is not None:
            print(f"BPM calculado: {bpm}")
            client.publish("/hr_pt", str(bpm))

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USER, PASS)
client.tls_set(ca_certs=CAFILE)
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT)
client.loop_forever()
