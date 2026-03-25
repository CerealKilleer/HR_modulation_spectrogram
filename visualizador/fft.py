import socket
import numpy as np
import matplotlib.pyplot as plt

# ==============================
# Parámetros físicos
# ==============================

FS = 256

NFFT = 512
FREQ_BINS = NFFT // 2 + 1
MOD_BINS = FREQ_BINS

HOP = 8

FS_MOD = FS / HOP
FM_MAX = FS_MOD / 2

TOTAL_FLOATS = FREQ_BINS * MOD_BINS
TOTAL_BYTES = TOTAL_FLOATS * 4

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
# Ejes
# ==============================

freq_axis = np.linspace(0, FS/2, FREQ_BINS)
fm_axis = np.linspace(0, FM_MAX, MOD_BINS)

# ==============================
# Inicializar gráfico
# ==============================

plt.ion()  # 🔥 modo interactivo

fig, ax = plt.subplots(figsize=(8, 5))

# imagen inicial vacía
mod_spec_db = np.zeros((FREQ_BINS, MOD_BINS))

img = ax.imshow(
    mod_spec_db,
    aspect='auto',
    origin='lower',
    extent=[fm_axis[0], fm_axis[-1], freq_axis[0], freq_axis[-1]],
    cmap='jet',
    vmin=-40,
    vmax=0
)

plt.colorbar(img, ax=ax, label="Energía (dB)")
ax.set_xlabel("Frecuencia de modulación (Hz)")
ax.set_ylabel("Frecuencia (Hz)")
ax.set_title("Espectrograma de modulación (streaming)")

plt.tight_layout()

# ==============================
# Loop continuo
# ==============================

while True:
    data = b''
    while len(data) < TOTAL_BYTES:
        packet = conn.recv(TOTAL_BYTES - len(data))
        if not packet:
            raise RuntimeError("Conexión cerrada")
        data += packet

    mod_spec = np.frombuffer(data, dtype=np.float32)
    mod_spec = mod_spec.reshape(FREQ_BINS, MOD_BINS)

    # log + normalización
    mod_spec_db = 10 * np.log10(mod_spec + 1e-12)
    mod_spec_db -= np.max(mod_spec_db)

    # 🔥 actualizar imagen
    img.set_data(mod_spec_db)

    plt.pause(0.001)  # refresco
