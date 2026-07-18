import os
import json
import tkinter as tk
from tkinter import messagebox
from jinja2 import Environment, FileSystemLoader

# --- THIẾT LẬP ĐƯỜNG DẪN ---
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
JSON_FILE_PATH = os.path.join(BASE_DIR, "config_data.json")
TEMPLATE_DIR = os.path.join(BASE_DIR, "../templates")
OUTPUT_DIR = os.path.join(BASE_DIR, "../../Test/Generated_File")

class AUTOSAR_CodeGen_Tool:
    def __init__(self, root):
        self.root = root
        self.root.title("AUTOSAR Mem Configuration Tool")
        self.root.geometry("400x450")
        
        self.config_data = {}
        self.entries = {}
        
        # Load data cu tu JSON neu co
        self.load_json_data()
        
        # Ve giao dien
        self.create_gui()

    def load_json_data(self):
        """Doc cau hinh (Muc 1.5)"""
        if os.path.exists(JSON_FILE_PATH):
            with open(JSON_FILE_PATH, 'r', encoding='utf-8') as f:
                self.config_data = json.load(f)
        else:
            # Gia tri mac dinh theo Requirement (Muc 1.4)
            self.config_data = {
                "MemDevErrorDetect": "STD_ON",
                "MemInstanceId": 0,
                "MemStartAddress": "0x08000000",
                "MemNumberOfSectors": 64,
                "MemEraseSectorSize": 2048,
                "MemWritePageSize": 4,
                "MemMinReadSize": 1
            }

    def create_gui(self):
        """Tao giao dien nhap lieu """
        tk.Label(self.root, text="AUTOSAR Mem Parameters", font=("Arial", 14, "bold")).pack(pady=10)
        
        frame = tk.Frame(self.root)
        frame.pack(padx=20, fill="both", expand=True)

        row = 0
        for key, value in self.config_data.items():
            tk.Label(frame, text=key + ":", anchor="w").grid(row=row, column=0, sticky="w", pady=5)
            entry = tk.Entry(frame, width=25)
            entry.insert(0, str(value))
            entry.grid(row=row, column=1, pady=5)
            self.entries[key] = entry
            row += 1

        btn_frame = tk.Frame(self.root)
        btn_frame.pack(pady=20)
        
        btn_save = tk.Button(btn_frame, text="Save Config", width=12, command=self.save_config)
        btn_save.grid(row=0, column=0, padx=10)
        
        btn_gen = tk.Button(btn_frame, text="Generate Code", width=12, bg="green", fg="white", command=self.generate_code)
        btn_gen.grid(row=0, column=1, padx=10)

    def save_config(self):
        """Luu giu giao dien vao file JSON"""
        for key in self.entries:
            self.config_data[key] = self.entries[key].get()
            
        with open(JSON_FILE_PATH, 'w', encoding='utf-8') as f:
            json.dump(self.config_data, f, indent=4)
        messagebox.showinfo("Success", "Configuration Saved to JSON!")

    def generate_code(self):
        """Sinh file C/H chuan tu template"""
        self.save_config() # Luu lai truoc khi sinh code
        
        if not os.path.exists(OUTPUT_DIR):
            os.makedirs(OUTPUT_DIR)

        try:
            env = Environment(loader=FileSystemLoader(TEMPLATE_DIR))
            
            # --- 1. RENDER FILE Mem_Cfg.h ---
            header_template_mem = env.get_template("include/Mem_Cfg.h.template")
            header_output_mem = header_template_mem.render(self.config_data)
            
            output_file_mem = os.path.join(OUTPUT_DIR, "Mem_Cfg.h")
            with open(output_file_mem, 'w', encoding='utf-8') as f:
                f.write(header_output_mem)

            # --- 2. RENDER FILE Flash_IP_Cfg.h ---
            header_template_ip = env.get_template("include/Flash_IP_Cfg.h.template")
            header_output_ip = header_template_ip.render(self.config_data)
            
            output_file_ip = os.path.join(OUTPUT_DIR, "Flash_IP_Cfg.h")
            with open(output_file_ip, 'w', encoding='utf-8') as f:
                f.write(header_output_ip)
                
            messagebox.showinfo("Success", f"Đã sinh code thành công 2 file vào thư mục:\n{OUTPUT_DIR}")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to generate code:\n{e}")

if __name__ == "__main__":
    root = tk.Tk()
    app = AUTOSAR_CodeGen_Tool(root)
    root.mainloop()