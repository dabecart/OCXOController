import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

def distorted_checkerboard(x, y, width, height, tile_size, time):
    # Center of the screen
    cx = width / 2.0
    cy = height / 2.0

    # Distance from center
    dx = x - cx
    dy = y - cy
    dist = np.sqrt(dx ** 2 + dy ** 2)

    # Ripple parameters
    frequency = 0.15  # number of ripples
    amplitude = 2.0   # distortion strength
    speed = 2.0       # animation speed

    # Ripple effect
    ripple = np.sin(dist * frequency - time * speed) * amplitude
    scale = ripple / (dist + 1e-5)  # avoid division by zero
    xd = x + dx * scale
    yd = y + dy * scale

    # Checkerboard pattern from distorted coordinates
    xi = (xd // tile_size).astype(int)
    yi = (yd // tile_size).astype(int)
    checker = ((xi + yi) % 2) * 255  # 0 or 255

    return checker.astype(np.uint8)

# === Configuration ===
WIDTH, HEIGHT = 128, 60  # or 240, 240 for your TFT
TILE_SIZE = 8
FPS = 30
DURATION_SEC = 10

# === Setup coordinate grid ===
x, y = np.meshgrid(np.arange(WIDTH), np.arange(HEIGHT))

# === Plot setup ===
fig, ax = plt.subplots()
fig.canvas.manager.set_window_title("TFT Simulator")
img = ax.imshow(np.zeros((HEIGHT, WIDTH)), cmap='gray', vmin=0, vmax=255, interpolation='nearest')
ax.axis('off')

# === Animation function ===
def update(frame):
    t = frame / FPS
    frame_data = distorted_checkerboard(x, y, WIDTH, HEIGHT, TILE_SIZE, t)
    img.set_array(frame_data)
    return [img]

# === Run animation ===
frame_count = DURATION_SEC * FPS
ani = animation.FuncAnimation(fig, update, frames=frame_count, interval=1000 // FPS, blit=True)
# plt.show()

# === Optional: Save to MP4 ===
# import matplotlib as mpl 
# mpl.rcParams['animation.ffmpeg_path'] = r"D:\\Descargas\\ffmpeg-2025-07-12-git-35a6de137a-essentials_build\\bin\\ffmpeg.exe"
# FFwriter = animation.FFMpegWriter(fps=FPS)
# ani.save("checkerboard_ripple.mp4", writer=FFwriter)
