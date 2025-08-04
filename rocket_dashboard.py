import tkinter as tk
from tkinter import ttk
import serial
import threading
import time
import ast
from collections import deque
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

PORT = "COM4"
BAUDRATE = 115200
port = serial.Serial(PORT, BAUDRATE, timeout=0.1)
time_data = deque()
altitude_data = deque()
data_lock = threading.Lock()
start_time = None

root = tk.Tk()
root.title("ROCKET DASHBOARD - ROCKTOCOCK-1")

fig, ax = plt.subplots(figsize=(8, 4))
line, = ax.plot([], [], color="blue", lw=2, label="Altitude (m)")
ax.set_title("Altitude")
ax.set_xlabel("Time (s)")
ax.set_ylabel("Altitude (m)")
ax.set_xlim(-30, 0)
ax.set_ylim(0, 100)
ax.grid(True)
ax.legend()

canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

status_frame = ttk.Frame(root)
status_frame.pack(fill=tk.X, padx=10, pady=10)

altitude_label = ttk.Label(status_frame, text="  Altitude: --- m  ", font=("Arial", 12))
altitude_label.grid(row=0, column=0, padx=20)


pitch_label = ttk.Label(status_frame, text="  Peak Altitude: --- m  ", font=("Arial", 12))
pitch_label.grid(row=0, column=2, padx=20)

elapsed_label = ttk.Label(status_frame, text="  Flight Time: 00:00:00  ", font=("Arial", 12))
elapsed_label.grid(row=0, column=4, padx=20)

light_frame = ttk.Frame(root)
light_frame.pack(pady=10)

light_canvases = []
lights = []
light_labels = ["Parachute deployed", "Apogee detected", "Parachute armed"]

for i in range(3):
    light_canvas = tk.Canvas(light_frame, width=30, height=30)
    light = light_canvas.create_oval(5, 5, 25, 25, fill="red")
    light_canvas.grid(row=0, column=i, padx=15)
    ttk.Label(light_frame, text=light_labels[i]).grid(row=1, column=i)
    light_canvases.append(light_canvas)
    lights.append(light)

def read_from_port():
    global start_time
    buffer = ""
    with open("log.txt", "a") as log_file:
        while True:
            if port.in_waiting > 0:
                data = port.read(port.in_waiting).decode(errors="ignore")
                buffer += data
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    line = line.strip()
                    timestamp_str = time.strftime('%Y-%m-%d %H:%M:%S')
                    print(f"{timestamp_str} ESP32: {line}")
                    log_file.write(f"{timestamp_str} {line}\n")
                    log_file.flush()

                    try:
                        values = ast.literal_eval(line)
                        if isinstance(values, list) and len(values) >= 7:
                            esp_time = float(values[0])
                            roll = float(values[1])
                            altitude = float(values[2])
                            pitch = float(values[3])
                            button1 = bool(values[4])
                            button2 = bool(values[5])
                            button3 = bool(values[6])

                            pc_time = time.time()
                            with data_lock:
                                if start_time is None:
                                    start_time = pc_time
                                time_data.append(pc_time)
                                altitude_data.append(altitude)
                                while time_data and (pc_time - time_data[0]) > 30:
                                    time_data.popleft()
                                    altitude_data.popleft()

                            root.after(0, update_status_display, altitude, roll, pitch, button1, button2, button3)
                    except Exception:
                        continue

def write_to_port():
    while True:
        try:
            cmd = input()
            if cmd:
                port.write((cmd + '\n').encode())
        except KeyboardInterrupt:
            print("Exiting...")
            break

def update_plot():
    with data_lock:
        if len(time_data) >= 2:
            now = time.time()
            x_vals = [t - now for t in time_data]
            y_vals = list(altitude_data)

            line.set_data(x_vals, y_vals)

            ax.set_xlim(-30, 0)
            ymin = min(y_vals)
            ymax = max(y_vals)
            if ymin == ymax:
                ymax += 1
            ax.set_ylim(ymin - 5, ymax + 5)

            canvas.draw()
    root.after(500, update_plot)

def update_status_display(alt, roll, pitch, b1, b2, b3):
    altitude_label.config(text=f"Altitude: {alt:.2f} m")
    pitch_label.config(text=f"Peak Altitude: {pitch:.2f}m")

    for i, state in enumerate([b1, b2, b3]):
        color = "green" if state else "red"
        light_canvases[i].itemconfig(lights[i], fill=color)

def update_elapsed_time():
    if start_time is not None:
        elapsed = int(time.time() - start_time)
        hrs, rem = divmod(elapsed, 3600)
        mins, secs = divmod(rem, 60)
        elapsed_label.config(text=f"Flight Time: {hrs:02}:{mins:02}:{secs:02}")
    root.after(1000, update_elapsed_time)

reader_thread = threading.Thread(target=read_from_port, daemon=True)
reader_thread.start()

writer_thread = threading.Thread(target=write_to_port, daemon=True)
writer_thread.start()

update_plot()
update_elapsed_time()

root.mainloop()
