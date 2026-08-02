import tkinter as tk
from tkinter import ttk, messagebox, filedialog, simpledialog
import xml.etree.ElementTree as ET
import os

try:
    from jinja2 import Environment, FileSystemLoader
    JINJA_AVAILABLE = True
except ImportError:
    JINJA_AVAILABLE = False

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
TEMPLATE_DIR = os.path.abspath(os.path.join(BASE_DIR, "../templates/include"))
OUTPUT_DIR = os.path.abspath(os.path.join(BASE_DIR, "../../../Test/Generated_File"))
EPD_FILE = os.path.abspath(os.path.join(BASE_DIR, "Fls_s32k118_lqfp48.epd"))

class MemConfiguratorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("AUTOSAR Mem Driver Configurator (EPD/EPC)")
        self.root.geometry("950x850")
        
        self.epd_tree = None
        self.ui_vars = {} # Maps container_name -> { param_name: tk.Variable }
        self.list_data = {} # Maps container_name -> { list_name: [ dicts ] }
        self.trees = {} # Maps (c_name, l_name) -> (tree_widget, headers_list)
        
        self.load_epd(EPD_FILE)
        self.setup_ui()

    def _parse_arxml_to_custom_tree(self, epd_path):
        tree = ET.parse(epd_path)
        root = tree.getroot()
        ns = {'ns': 'http://autosar.org/schema/r4.0'}
        
        module_def = root.find('.//ns:ECUC-MODULE-DEF', ns)
        if module_def is None:
            raise ValueError("No ECUC-MODULE-DEF found in ARXML.")
            
        mod_name_elem = module_def.find('ns:SHORT-NAME', ns)
        mod_name = mod_name_elem.text if mod_name_elem is not None else "UnknownModule"
        
        custom_root = ET.Element("AUTOSAR_EPD", version="4.4.0")
        custom_mod = ET.SubElement(custom_root, "Module", name=mod_name)
        
        for container in module_def.findall('.//ns:ECUC-PARAM-CONF-CONTAINER-DEF', ns):
            c_name_elem = container.find('ns:SHORT-NAME', ns)
            if c_name_elem is None:
                continue
            c_name = c_name_elem.text
            custom_cont = ET.SubElement(custom_mod, "Container", name=c_name)
            
            for param in container.findall('.//*', ns):
                if param.tag.endswith('-PARAM-DEF'):
                    p_name_elem = param.find('ns:SHORT-NAME', ns)
                    if p_name_elem is None:
                        continue
                    p_name = p_name_elem.text
                    
                    p_type = "STRING"
                    if "BOOLEAN" in param.tag: p_type = "BOOLEAN"
                    elif "INTEGER" in param.tag: p_type = "INTEGER"
                    elif "FLOAT" in param.tag: p_type = "FLOAT"
                    elif "ENUMERATION" in param.tag: p_type = "ENUM"
                    
                    default_val = ""
                    def_elem = param.find('ns:DEFAULT-VALUE', ns)
                    if def_elem is not None and def_elem.text:
                        default_val = def_elem.text
                        
                    options = []
                    if p_type == "ENUM":
                        for lit in param.findall('.//ns:ECUC-ENUMERATION-LITERAL-DEF/ns:SHORT-NAME', ns):
                            if lit.text:
                                options.append(lit.text)
                    
                    attribs = {"name": p_name, "type": p_type, "default": default_val}
                    if options:
                        attribs["options"] = ",".join(options)
                        
                    ET.SubElement(custom_cont, "Parameter", **attribs)
                    
        return ET.ElementTree(custom_root)

    def load_epd(self, epd_path):
        if not os.path.exists(epd_path):
            messagebox.showerror("Error", f"EPD file not found at:\n{epd_path}")
            return
        try:
            temp_tree = ET.parse(epd_path)
            root_tag = temp_tree.getroot().tag
            
            if root_tag.endswith("AUTOSAR"):
                # Parse as ARXML
                self.epd_tree = self._parse_arxml_to_custom_tree(epd_path)
            else:
                # Parse as Custom XML (AUTOSAR_EPD)
                self.epd_tree = temp_tree
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load EPD file:\n{e}")
            
    def setup_ui(self):
        if not self.epd_tree:
            return
            
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        canvas = tk.Canvas(main_frame)
        scrollbar = ttk.Scrollbar(main_frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
        root_elem = self.epd_tree.getroot()
        module = root_elem.find("Module")
        if module is None:
            messagebox.showerror("Error", "No Module tag found in EPD")
            return
            
        for container in module.findall("Container"):
            c_name = container.get("name")
            c_desc = container.get("description", "")
            
            frame = ttk.LabelFrame(scrollable_frame, text=f"{c_name} - {c_desc}")
            frame.pack(fill="x", padx=10, pady=10, expand=True)
            
            self.ui_vars[c_name] = {}
            row_idx = 0
            
            # Parameters
            for param in container.findall("Parameter"):
                p_name = param.get("name")
                p_type = param.get("type", "STRING")
                p_default = param.get("default", "")
                p_desc = param.get("description", "")
                
                ttk.Label(frame, text=p_name + ":").grid(row=row_idx, column=0, padx=5, pady=5, sticky="e")
                
                if p_type == "BOOLEAN":
                    var = tk.StringVar(value=p_default)
                    cb = ttk.Checkbutton(frame, text=p_desc, variable=var, onvalue="true", offvalue="false")
                    cb.grid(row=row_idx, column=1, padx=5, pady=5, sticky="w")
                    self.ui_vars[c_name][p_name] = var
                elif p_type == "ENUM":
                    var = tk.StringVar(value=p_default)
                    options = param.get("options", "").split(",")
                    cb = ttk.Combobox(frame, textvariable=var, values=options, state="readonly", width=25)
                    cb.grid(row=row_idx, column=1, padx=5, pady=5, sticky="w")
                    self.ui_vars[c_name][p_name] = var
                else:
                    var = tk.StringVar(value=p_default)
                    readonly = param.get("readonly") == "true"
                    entry = ttk.Entry(frame, textvariable=var, width=30)
                    if readonly:
                        entry.config(state="readonly")
                    entry.grid(row=row_idx, column=1, padx=5, pady=5, sticky="w")
                    
                    unit = param.get("unit")
                    if unit:
                        ttk.Label(frame, text=unit).grid(row=row_idx, column=2, padx=5, pady=5, sticky="w")
                        
                    self.ui_vars[c_name][p_name] = var
                    
                row_idx += 1
                
            # Lists
            for lst in container.findall("List"):
                l_name = lst.get("name")
                
                if c_name not in self.list_data:
                    self.list_data[c_name] = {}
                
                items_data = []
                headers = set()
                for item in lst.findall("Item"):
                    items_data.append(item.attrib.copy())
                    for k in item.attrib.keys():
                        headers.add(k)
                        
                self.list_data[c_name][l_name] = items_data
                
                if not headers:
                    continue
                    
                headers = sorted(list(headers))
                
                tree_frame = ttk.Frame(frame)
                tree_frame.grid(row=row_idx, column=0, columnspan=3, padx=5, pady=5, sticky="we")
                
                ttk.Label(tree_frame, text=l_name).pack(anchor="w")
                
                tree = ttk.Treeview(tree_frame, columns=headers, show="headings", height=8, selectmode="browse")
                for h in headers:
                    tree.heading(h, text=h)
                    tree.column(h, width=120, anchor="center")
                    
                for item_dict in items_data:
                    row_vals = [item_dict.get(h, "") for h in headers]
                    tree.insert("", "end", values=row_vals)
                    
                tree.pack(fill="x", expand=True)
                
                self.trees[(c_name, l_name)] = (tree, headers)
                
                btn_frame = ttk.Frame(tree_frame)
                btn_frame.pack(fill="x", pady=5)
                
                ttk.Button(btn_frame, text="Add Item", command=lambda c=c_name, l=l_name: self.add_tree_item(c, l)).pack(side="left", padx=5)
                ttk.Button(btn_frame, text="Edit Selected", command=lambda c=c_name, l=l_name: self.edit_tree_item(c, l)).pack(side="left", padx=5)
                ttk.Button(btn_frame, text="Delete Selected", command=lambda c=c_name, l=l_name: self.delete_tree_item(c, l)).pack(side="left", padx=5)
                
                # Double click to edit
                tree.bind("<Double-1>", lambda event, c=c_name, l=l_name: self.edit_tree_item(c, l))

                row_idx += 1
                
        # Action Buttons
        frame_actions = ttk.Frame(self.root)
        frame_actions.pack(padx=10, pady=10, fill="x")

        ttk.Button(frame_actions, text="Save EPC", command=self.save_epc).pack(side="left", padx=5)
        ttk.Button(frame_actions, text="Load EPC", command=self.load_epc).pack(side="left", padx=5)
        ttk.Button(frame_actions, text="Generate Code", command=self.generate_code).pack(side="right", padx=5)

    def add_tree_item(self, c_name, l_name):
        tree, headers = self.trees[(c_name, l_name)]
        
        edit_win = tk.Toplevel(self.root)
        edit_win.title("Add New Item")
        edit_win.geometry("400x400")
        edit_win.grab_set()
        
        entries = {}
        for idx, h in enumerate(headers):
            ttk.Label(edit_win, text=h + ":").grid(row=idx, column=0, padx=10, pady=5, sticky="e")
            var = tk.StringVar(value="")
            if h == "write_protect":
                var.set("false")
                ttk.Checkbutton(edit_win, variable=var, onvalue="true", offvalue="false").grid(row=idx, column=1, padx=10, pady=5, sticky="w")
            else:
                ttk.Entry(edit_win, textvariable=var, width=30).grid(row=idx, column=1, padx=10, pady=5, sticky="w")
            entries[h] = var
            
        def save_new():
            new_vals = [entries[h].get() for h in headers]
            tree.insert("", "end", values=new_vals)
            edit_win.destroy()
            
        ttk.Button(edit_win, text="Add", command=save_new).grid(row=len(headers), column=0, columnspan=2, pady=15)

    def edit_tree_item(self, c_name, l_name):
        tree, headers = self.trees[(c_name, l_name)]
        selected = tree.selection()
        if not selected:
            messagebox.showwarning("Warning", "Please select an item to edit.")
            return
            
        item_id = selected[0]
        current_vals = tree.item(item_id, "values")
        
        edit_win = tk.Toplevel(self.root)
        edit_win.title("Edit Item")
        edit_win.geometry("400x400")
        edit_win.grab_set()
        
        entries = {}
        for idx, h in enumerate(headers):
            ttk.Label(edit_win, text=h + ":").grid(row=idx, column=0, padx=10, pady=5, sticky="e")
            val = current_vals[idx] if idx < len(current_vals) else ""
            var = tk.StringVar(value=val)
            if h == "write_protect":
                if not val: var.set("false")
                ttk.Checkbutton(edit_win, variable=var, onvalue="true", offvalue="false").grid(row=idx, column=1, padx=10, pady=5, sticky="w")
            else:
                ttk.Entry(edit_win, textvariable=var, width=30).grid(row=idx, column=1, padx=10, pady=5, sticky="w")
            entries[h] = var
            
        def save_edit():
            new_vals = [entries[h].get() for h in headers]
            tree.item(item_id, values=new_vals)
            edit_win.destroy()
            
        ttk.Button(edit_win, text="Save", command=save_edit).grid(row=len(headers), column=0, columnspan=2, pady=15)

    def delete_tree_item(self, c_name, l_name):
        tree, headers = self.trees[(c_name, l_name)]
        selected = tree.selection()
        if not selected:
            return
        if messagebox.askyesno("Confirm", "Are you sure you want to delete this item?"):
            tree.delete(selected[0])

    def get_tree_data(self, c_name, l_name):
        tree, headers = self.trees[(c_name, l_name)]
        items = []
        for child in tree.get_children():
            vals = tree.item(child, "values")
            item_dict = {h: str(v) for h, v in zip(headers, vals)}
            items.append(item_dict)
        return items

    def save_epc(self):
        filepath = filedialog.asksaveasfilename(initialdir=BASE_DIR, initialfile="MemDriver.epc", defaultextension=".epc", filetypes=[("EPC XML Files", "*.epc"), ("All Files", "*.*")])
        if not filepath:
            return
            
        root = ET.Element("AUTOSAR_EPC")
        module_epd = self.epd_tree.getroot().find("Module")
        mod_name = module_epd.get("name", "MemDriver") if module_epd is not None else "MemDriver"
        
        module = ET.SubElement(root, "Module", name=mod_name)
        
        for c_name, params in self.ui_vars.items():
            container = ET.SubElement(module, "Container", name=c_name)
            for p_name, var in params.items():
                ET.SubElement(container, "Parameter", name=p_name, value=var.get())
                
            # Save Lists from UI trees instead of static data
            for (tc_name, tl_name), (tree, headers) in self.trees.items():
                if tc_name == c_name:
                    lst = ET.SubElement(container, "List", name=tl_name)
                    items_data = self.get_tree_data(c_name, tl_name)
                    for item_dict in items_data:
                        ET.SubElement(lst, "Item", **item_dict)
                        
        tree = ET.ElementTree(root)
        try:
            tree.write(filepath, encoding="UTF-8", xml_declaration=True)
            messagebox.showinfo("Success", f"EPC saved to {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save EPC:\n{e}")

    def load_epc(self):
        filepath = filedialog.askopenfilename(initialdir=BASE_DIR, filetypes=[("EPC XML Files", "*.epc"), ("All Files", "*.*")])
        if not filepath:
            return
            
        try:
            epc_tree = ET.parse(filepath)
            root = epc_tree.getroot()
            module = root.find("Module")
            if module is None:
                raise ValueError("Invalid EPC: No Module tag")
                
            for container in module.findall("Container"):
                c_name = container.get("name")
                if c_name in self.ui_vars:
                    # Load Parameters
                    for param in container.findall("Parameter"):
                        p_name = param.get("name")
                        if p_name in self.ui_vars[c_name]:
                            self.ui_vars[c_name][p_name].set(param.get("value", ""))
                    
                    # Load Lists
                    for lst in container.findall("List"):
                        l_name = lst.get("name")
                        if (c_name, l_name) in self.trees:
                            tree, headers = self.trees[(c_name, l_name)]
                            # clear tree
                            for item in tree.get_children():
                                tree.delete(item)
                            
                            # insert loaded items
                            for item_elem in lst.findall("Item"):
                                row_vals = [item_elem.get(h, "") for h in headers]
                                tree.insert("", "end", values=row_vals)
                            
            messagebox.showinfo("Success", f"EPC loaded from {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load EPC:\n{e}")

    def generate_code(self):
        if not JINJA_AVAILABLE:
            messagebox.showerror("Error", "Thư viện 'jinja2' chưa được cài đặt!\nVui lòng chạy lệnh: pip install jinja2")
            return

        # Build config dictionary for Jinja2
        config = {}
        for c_name, params in self.ui_vars.items():
            config[c_name] = {}
            for p_name, var in params.items():
                val = var.get()
                if val == "true":
                    val = "STD_ON"
                elif val == "false":
                    val = "STD_OFF"
                config[c_name][p_name] = val
                
            # Get list data directly from Treeviews
            for (tc_name, tl_name), (tree, headers) in self.trees.items():
                if tc_name == c_name:
                    config[tl_name] = self.get_tree_data(c_name, tl_name)

        if not os.path.exists(OUTPUT_DIR):
            os.makedirs(OUTPUT_DIR)

        try:
            env = Environment(loader=FileSystemLoader(TEMPLATE_DIR))
            
            template_mem = env.get_template("Mem_Cfg.h.template")
            mem_content = template_mem.render(config=config)
            with open(os.path.join(OUTPUT_DIR, "Mem_Cfg.h"), 'w', encoding='utf-8') as f:
                f.write(mem_content)
                
            template_flash = env.get_template("Flash_IP_Cfg.h.template")
            flash_content = template_flash.render(config=config)
            with open(os.path.join(OUTPUT_DIR, "Flash_IP_Cfg.h"), 'w', encoding='utf-8') as f:
                f.write(flash_content)

            messagebox.showinfo("Thành công", f"Đã generate code thành công tại:\n{OUTPUT_DIR}")
        except Exception as e:
            messagebox.showerror("Lỗi Generate", f"Lỗi trong quá trình generate: {e}")

if __name__ == "__main__":
    root = tk.Tk()
    app = MemConfiguratorApp(root)
    root.mainloop()
