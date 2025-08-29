import tkinter as tk
import tkinter.font as tkFont
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading

class SerialMonitor:
    def __init__(self, root):
        self.root = root
        self.root.title("Serial Monitor password manager")
        self.serial_connection = None
        self.stop_thread = False
        self.custom_font = tkFont.Font(family="Consolas", size=14)  # or "Courier New"

        # --- Serial Monitor Display ---
        self.text_area = tk.Text(root, height=20, width=70, state="disabled", wrap="word",
                         font=self.custom_font)
        self.text_area.grid(row=0, column=0, columnspan=4, padx=5, pady=5)
        self.text_area.config(background="black", foreground="#39FF14")
           

        # --- COM Port Selector ---
        ttk.Label(root, text="COM Port:").grid(row=1, column=0, sticky="e")
        self.port_var = tk.StringVar()
        self.port_menu = ttk.Combobox(root, textvariable=self.port_var, state="readonly")
        self.port_menu['values'] = self.get_serial_ports()
        self.port_menu.grid(row=1, column=1, padx=5, pady=5)

        # --- Connect / Disconnect Button ---
        self.connect_button = ttk.Button(root, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=1, column=2, padx=5, pady=5)

        # --- Input Entry (masked by default) ---
        self.input_var = tk.StringVar()
        self.entry = ttk.Entry(root, textvariable=self.input_var, show="*", font=self.custom_font)
        self.entry.grid(row=2, column=0, columnspan=2, padx=5, pady=5, sticky="we")
        self.entry.bind("<Return>", self.send_data)  # <--- Bind Enter key
        self.entry.config(background="black", foreground="black")

        # --- Show / Hide toggle button ---
        self.showing = False
        self.show_button = ttk.Button(root, text="Show", command=self.toggle_show)
        self.show_button.grid(row=2, column=2, padx=5, pady=5)

        # --- Character Counter ---
        self.char_count_label = tk.Label(root, text="0", fg="green", font=("Consolas", 12))
        self.char_count_label.grid(row=2, column=1, padx=5, pady=5)

        # --- Send Button ---
        self.send_button = ttk.Button(root, text="Send", command=self.send_data)
        self.send_button.grid(row=2, column=3, padx=5, pady=5)

        # --- Bind typing event to update char counter ---
        self.input_var.trace_add("write", self.update_char_count)

        root.protocol("WM_DELETE_WINDOW", self.on_close)

    def get_serial_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def toggle_connection(self):
        if self.serial_connection and self.serial_connection.is_open:
            # Disconnect
            self.stop_thread = True
            self.serial_connection.close()
            self.serial_connection = None
            self.connect_button.config(text="Connect")
            self.append_text("Disconnected\n")
        else:
            # Connect
            port = self.port_var.get()
            if not port:
                messagebox.showwarning("Warning", "Please select a COM port.")
                return
            try:
                self.serial_connection = serial.Serial(port, 9600, timeout=1)
                self.stop_thread = False
                self.connect_button.config(text="Disconnect")
                self.append_text(f"Connected to {port}\n")

                # Start thread for reading serial data
                threading.Thread(target=self.read_serial, daemon=True).start()
            except Exception as e:
                messagebox.showerror("Error", str(e))

    def read_serial(self):
        while not self.stop_thread and self.serial_connection and self.serial_connection.is_open:
            try:
                line = self.serial_connection.readline().decode(errors="ignore")
                if line:
                    self.append_text(line)
            except Exception:
                break

    def append_text(self, msg):
        self.text_area.config(state="normal")
        self.text_area.insert("end", msg)
        self.text_area.see("end")
        self.text_area.config(state="disabled")

    def send_data(self, event=None):
        if self.serial_connection and self.serial_connection.is_open:
            data = self.input_var.get()
            if data:
                self.serial_connection.write(data.encode())  # no line ending
                self.input_var.set("")  # clear after sending

    def toggle_show(self):
        if self.showing:
            self.entry.config(show="*")
            self.show_button.config(text="Show")
        else:
            self.entry.config(show="")
            self.show_button.config(text="Hide")
        self.showing = not self.showing

    def on_close(self):
        self.stop_thread = True
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
        self.root.destroy()
        
    def update_char_count(self, *args):
        length = len(self.input_var.get())
        self.char_count_label.config(text=str(length))
        if length > 14:
            self.char_count_label.config(fg="red")
        else:
            self.char_count_label.config(fg="green")    


if __name__ == "__main__":
    root = tk.Tk()
    app = SerialMonitor(root)
    root.mainloop()
