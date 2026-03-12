import matplotlib
matplotlib.use('QtAgg')  # Backend rápido con PyQt6

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import time

# =====================
# CONFIGURACIÓN
# =====================

PORT = "/dev/ttyUSB0"
BAUD = 115200

ADC_MAX = 4095.0
VREF = 3.3

SAMPLE_RATE = 256          # Hz efectivos
WINDOW_SECONDS = 4
BUFFER_SIZE = SAMPLE_RATE * WINDOW_SECONDS

SMOOTHING = 1              # Promedio móvil (1 = sin suavizado)

# =====================
# BUFFER DE DATOS
# =====================

data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)
smooth_buffer = deque(maxlen=SMOOTHING)

# =====================
# SERIAL
# =====================

ser = serial.Serial(PORT, BAUD, timeout=0.01)

# Esperar estabilización
time.sleep(2)

# =====================
# FIGURA
# =====================

fig, ax = plt.subplots()

line, = ax.plot(data)

ax.set_ylim(-1.5, 1.5)
ax.set_xlim(0, BUFFER_SIZE)

ax.set_title("ECG en tiempo real")
ax.set_xlabel("Muestras")
ax.set_ylabel("Voltaje (V)")

ax.grid(True)

# =====================
# CONVERSIÓN ADC → ECG
# =====================

def adc_to_voltage(adc_value):

    voltage = (adc_value / ADC_MAX) * VREF

    # Centrar ECG en 0 V
    ecg = voltage - (VREF / 2.0)

    return ecg

# =====================
# UPDATE LOOP
# =====================

def update(frame):

    updated = False

    while ser.in_waiting:

        try:

            line_bytes = ser.readline()

            if not line_bytes:
                break

            adc_value = int(line_bytes.decode().strip())

            ecg_voltage = adc_to_voltage(adc_value)

            # Suavizado opcional
            smooth_buffer.append(ecg_voltage)
            ecg_smoothed = sum(smooth_buffer) / len(smooth_buffer)

            data.append(ecg_smoothed)

            updated = True

        except:
            pass

    if updated:
        line.set_ydata(data)

    return (line,)

# =====================
# ANIMACIÓN
# =====================

ani = animation.FuncAnimation(
    fig,
    update,
    interval=10,   # ms (100 FPS máximo)
    blit=True,
    cache_frame_data=False
)

# =====================
# EJECUTAR
# =====================

plt.show()

# =====================
# LIMPIEZA
# =====================

ser.close()
