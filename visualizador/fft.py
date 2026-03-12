import socket
import numpy as np
import matplotlib.pyplot as plt

# ==============================
# Parámetros físicos
# ==============================

FS = 256                 # Frecuencia de muestreo (Hz)
DURATION = 4             # segundos
N_SAMPLES = FS * DURATION

NFFT = 512               # porque FREQ_BINS = NFFT/2 + 1
FREQ_BINS = NFFT // 2 + 1
N_FRAMES = 125

FRAME_BYTES = FREQ_BINS * 4  # float32 = 4 bytes

HOST = "0.0.0.0"
PORT = 1234

# ==============================
# Servidor TCP
# ==============================

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen(1)

print("Esperando conexión...")
conn, addr = server.accept()
print("Conectado desde", addr)

# ==============================
# Buffer del espectrograma
# ==============================

spec = np.zeros((N_FRAMES, FREQ_BINS), dtype=np.float32)

# ==============================
# Recepción frame por frame
# ==============================

for frame in range(N_FRAMES):

    data = b''
    while len(data) < FRAME_BYTES:
        packet = conn.recv(FRAME_BYTES - len(data))
        if not packet:
            raise RuntimeError("Conexión cerrada inesperadamente")
        data += packet

    spec[frame, :] = np.frombuffer(data, dtype=np.float32)

print("Espectrograma recibido correctamente")

conn.close()
server.close()

# ==============================
# Construcción de ejes físicos
# ==============================

freq_axis = np.linspace(0, FS/2, FREQ_BINS)
time_axis = np.linspace(0, DURATION, N_FRAMES)

# ==============================
# Visualización
# ==============================

plt.figure(figsize=(8, 5))

plt.imshow(
    spec.T,
    aspect='auto',
    origin='lower',
    extent=[0, DURATION, 0, FS/2]
)

plt.xlabel("Tiempo (s)")
plt.ylabel("Frecuencia (Hz)")
plt.colorbar(label="Magnitud")
plt.title("Espectrograma recibido por TCP")
plt.tight_layout()
plt.show()
