import tkinter as tk
from tkinter import scrolledtext, messagebox
import socket
import threading


class TCPConsoleApp:
    def __init__(self, master, ip, port):
        self.master = master
        self.master.title("TCP Console")

        # Text area for messages
        self.text_area = scrolledtext.ScrolledText(master, wrap=tk.WORD, state='disabled', height=20)
        self.text_area.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)

        # Frame for input and send button
        input_frame = tk.Frame(master)
        input_frame.pack(padx=10, pady=5, fill=tk.X)

        self.entry = tk.Entry(input_frame)
        self.entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.entry.bind("<Return>", self.send_message)

        send_button = tk.Button(input_frame, text="Send", command=self.send_message)
        send_button.pack(side=tk.LEFT, padx=(5, 0))

        self.sock = None
        self.stop_event = threading.Event()

        # Start connection
        self.connect(ip, port)

    def connect(self, ip, port):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((ip, port))
            self.append_text(f"Connected to {ip}:{port}\n")
            threading.Thread(target=self.receive_loop, daemon=True).start()
        except Exception as e:
            messagebox.showerror("Connection Error", f"Could not connect: {e}")

    def append_text(self, message):
        self.text_area.config(state='normal')
        self.text_area.insert(tk.END, message)
        self.text_area.see(tk.END)
        self.text_area.config(state='disabled')

    def send_message(self, event=None):
        message = self.entry.get().strip()
        if message and self.sock:
            try:
                self.sock.sendall((message + "\r").encode())
                self.append_text(f"[Sent] {message}\n")
            except Exception as e:
                self.append_text(f"[Error] Could not send message: {e}\n")
        self.entry.delete(0, tk.END)

    def receive_loop(self):
        try:
            while not self.stop_event.is_set():
                data = self.sock.recv(1024)
                if not data:
                    self.append_text("[Disconnected]\n")
                    break
                self.append_text(data.decode(errors='replace'))
        except Exception as e:
            self.append_text(f"[Error] {e}\n")

    def on_close(self):
        self.stop_event.set()
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
        self.master.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    # Change IP and port here
    app = TCPConsoleApp(root, "192.168.1.30", 2314)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()
