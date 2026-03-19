import socket
import numpy as np
import matplotlib.pyplot as plt

# ==============================
# Parámetros físicos
# ==============================

FS = 256

NFFT = 512
FREQ_BINS = NFFT // 2 + 1        # 257
MOD_BINS = FREQ_BINS             # 257 (FFT de modulación)

HOP = 8

# Frecuencia de modulación
FS_MOD = FS / HOP                # 32 Hz
FM_MAX = FS_MOD / 2              # 16 Hz

TOTAL_FLOATS = FREQ_BINS * MOD_BINS
TOTAL_BYTES = TOTAL_FLOATS * 4   # float32

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
# Recepción matriz completa
# ==============================

data = b''
while len(data) < TOTAL_BYTES:
    packet = conn.recv(TOTAL_BYTES - len(data))
    if not packet:
        raise RuntimeError("Conexión cerrada inesperadamente")
    data += packet

mod_spec = np.frombuffer(data, dtype=np.float32)
mod_spec = mod_spec.reshape(FREQ_BINS, MOD_BINS)

print("Espectrograma de modulación recibido")

conn.close()
server.close()

# ==============================
# Ejes físicos
# ==============================

freq_axis = np.linspace(0, FS/2, FREQ_BINS)     # 0–128 Hz
fm_axis = np.linspace(0, FM_MAX, MOD_BINS)      # 0–16 Hz

# ==============================
# Escala log (CRÍTICO)
# ==============================

mod_spec_db = 10 * np.log10(mod_spec + 1e-12)

# ==============================
# Visualización
# ==============================

plt.figure(figsize=(8, 5))

plt.imshow(
    mod_spec_db,
    aspect='auto',
    origin='lower',
    extent=[fm_axis[0], fm_axis[-1], freq_axis[0], freq_axis[-1]],
    cmap='jet'
)

plt.xlabel("Frecuencia de modulación (Hz)")
plt.ylabel("Frecuencia (Hz)")
plt.title("Espectrograma de modulación")
plt.colorbar(label="Energía (dB)")

plt.tight_layout()
plt.show()
