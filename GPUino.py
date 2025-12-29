# ═══════════════════════════════════════════════════════════════
#                   GAMEPORT TO VJOY INTERFACE
#                  CODED BY PYARROZ
# ═══════════════════════════════════════════════════════════════
#
#                      .   ▄██▄
#                     ▄▀  ██████
#                    ▐▌  ████████▄
#                    ▐██▌▐████████
#                   ▄████▌███████
#                  ███████▐██████▌
#                 ████████▐██████▌
#                ▐████████▌██████
#                ▐████████▌█████▌
#                ▐████████▌█████
#                ▐████████▌████▌
#                ▐████████ ████
#               ▐█████████ ███▌
#              ███████████████▀
#             ████████████████
#            ██████████  ▀███
#          ▄███████████   ▐██▌
#         ▐█████████████▄  ███
#         ████████████████▄▐██▌
#        ██████████████████▌██▌
#       ▐██████████████████▌██
#       ███████████████████▌██
#       ███████████████████ ██
#       ███████████████████ ██
#       ██████████████████  ▀▀
#       ▀████████████████
#
# Serial → vJoy interface for Gameport joystick
# ═══════════════════════════════════════════════════════════════

import serial
import serial.tools.list_ports
import pyvjoy
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox

BAUD = 9600
VJOY_ID = 1

# ------------------ UTILITIES ------------------

def list_com_ports():
    return [p.device for p in serial.tools.list_ports.comports()]

def map_axis(val):
    return int(val * 32767 / 1023)

# ------------------ APP ------------------

class GPDuinoApp:
    def __init__(self, root):
        self.root = root
        self.root.title("GPDuino – Serial → vJoy")

        self.ser = None
        self.running = False

        # ----- UI -----
        frame = ttk.Frame(root, padding=10)
        frame.grid()

        ttk.Label(frame, text="COM Port:").grid(column=0, row=0, sticky="w")

        self.combobox = ttk.Combobox(
            frame, values=list_com_ports(), state="readonly", width=10
        )
        self.combobox.grid(column=1, row=0, padx=5)

        ttk.Button(frame, text="Refresh", command=self.refresh_ports).grid(column=2, row=0)

        self.btn_connect = ttk.Button(frame, text="Connect", command=self.toggle_connection)
        self.btn_connect.grid(column=0, row=1, columnspan=3, pady=5, sticky="ew")

        # ---- CHECKBOX SWAP AXES ----
        self.swap_axes = tk.BooleanVar(value=False)

        ttk.Checkbutton(
            frame,
            text="Swap Axis 1 ↔ Axis 2",
            variable=self.swap_axes
        ).grid(column=0, row=2, columnspan=3, sticky="w", pady=5)

        # ---- CHECKBOX ENABLE/DISABLE AXES ----
        self.enable_axis1 = tk.BooleanVar(value=True)
        self.enable_axis2 = tk.BooleanVar(value=True)

        ttk.Checkbutton(
            frame,
            text="Enable Axis 1 (X1, Y1)",
            variable=self.enable_axis1
        ).grid(column=0, row=3, columnspan=3, sticky="w", pady=2)

        ttk.Checkbutton(
            frame,
            text="Enable Axis 2 (X2, Y2)",
            variable=self.enable_axis2
        ).grid(column=0, row=4, columnspan=3, sticky="w", pady=2)

        self.log = tk.Text(frame, height=15, width=60, state="disabled")
        self.log.grid(column=0, row=5, columnspan=3, pady=5)

        self.refresh_ports()

    # ------------------ LOG ------------------

    def log_msg(self, msg):
        self.log.configure(state="normal")
        self.log.insert("end", msg + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    # ------------------ PORTS ------------------

    def refresh_ports(self):
        ports = list_com_ports()
        self.combobox["values"] = ports
        if ports:
            self.combobox.current(0)

    # ------------------ CONNECTION ------------------

    def toggle_connection(self):
        if self.running:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.combobox.get()
        if not port:
            messagebox.showerror("Error", "Select a COM port")
            return

        try:
            self.log_msg(f"[INFO] Connecting to {port}...")
            self.ser = serial.Serial(port, BAUD, timeout=1)
            time.sleep(2)

            self.j = pyvjoy.VJoyDevice(VJOY_ID)

            self.running = True
            self.btn_connect.config(text="Disconnect")

            self.thread = threading.Thread(target=self.read_loop, daemon=True)
            self.thread.start()

            self.log_msg("[OK] Connected and reading data")

        except Exception as e:
            self.log_msg(f"[ERROR] {e}")
            self.retry()

    def disconnect(self):
        self.running = False
        if self.ser:
            self.ser.close()
        self.btn_connect.config(text="Connect")
        self.log_msg("[INFO] Disconnected")

    def retry(self):
        self.log_msg("[INFO] Retrying in 3 seconds...")
        self.root.after(3000, self.connect)

    # ------------------ READING ------------------

    def read_loop(self):
        while self.running:
            try:
                raw = self.ser.readline()
                if not raw:
                    continue

                line = raw.decode("ascii", errors="ignore").strip()
                self.log_msg(f"[RAW] {line}")

                parts = line.split(",")
                if len(parts) != 8:
                    self.log_msg("[SKIP] Invalid format")
                    continue

                try:
                    # First 4 values are floats (axes), last 4 are ints (buttons)
                    x1 = float(parts[0])
                    y1 = float(parts[1])
                    x2 = float(parts[2])
                    y2 = float(parts[3])
                    b1 = int(parts[4])
                    b2 = int(parts[5])
                    b3 = int(parts[6])
                    b4 = int(parts[7])
                except ValueError:
                    self.log_msg("[SKIP] Not convertible to float/int")
                    continue

                # ---- AXIS SWAP ----
                if self.swap_axes.get():
                    x1, x2 = x2, x1
                    y1, y2 = y2, y1

                # ---- APPLY AXES IF ENABLED ----
                if self.enable_axis1.get():
                    self.j.set_axis(pyvjoy.HID_USAGE_X,  map_axis(x1))
                    self.j.set_axis(pyvjoy.HID_USAGE_Y,  map_axis(y1))
                else:
                    # Center axis 1 when disabled
                    self.j.set_axis(pyvjoy.HID_USAGE_X,  16383)
                    self.j.set_axis(pyvjoy.HID_USAGE_Y,  16383)

                if self.enable_axis2.get():
                    self.j.set_axis(pyvjoy.HID_USAGE_Z,  map_axis(x2))
                    self.j.set_axis(pyvjoy.HID_USAGE_RX, map_axis(y2))
                else:
                    # Center axis 2 when disabled
                    self.j.set_axis(pyvjoy.HID_USAGE_Z,  16383)
                    self.j.set_axis(pyvjoy.HID_USAGE_RX, 16383)

                self.j.set_button(1, b1)
                self.j.set_button(2, b2)
                self.j.set_button(3, b3)
                self.j.set_button(4, b4)

            except Exception as e:
                self.log_msg(f"[ERROR] Reading failed: {e}")
                self.running = False
                self.retry()
                break

# ------------------ MAIN ------------------

if __name__ == "__main__":
    root = tk.Tk()
    app = GPDuinoApp(root)
    root.mainloop()
