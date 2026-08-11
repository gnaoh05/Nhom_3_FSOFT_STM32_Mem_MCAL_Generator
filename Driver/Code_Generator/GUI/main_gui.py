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
INCLUDE_TEMPLATE_DIR = os.path.abspath(os.path.join(BASE_DIR, "../templates/include"))
SRC_TEMPLATE_DIR = os.path.abspath(os.path.join(BASE_DIR, "../templates/src"))
OUTPUT_DIR = os.path.abspath(os.path.join(BASE_DIR, "../../../Test/Generated_File"))
EPD_FILE = os.path.abspath(os.path.join(BASE_DIR, "MemDriver.epd"))

class MemConfiguratorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("AUTOSAR Mem Driver Configurator (EPD/EPC)")
        self.root.geometry("1050x900")
        
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
        
        # Bind mouse wheel for scrolling
        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        canvas.bind_all("<MouseWheel>", _on_mousewheel)
        
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
                    if p_desc:
                        ttk.Label(frame, text=p_desc).grid(row=row_idx, column=2, padx=5, pady=5, sticky="w")
                    self.ui_vars[c_name][p_name] = var
                else:
                    var = tk.StringVar(value=p_default)
                    readonly = param.get("readonly") == "true"
                    entry = ttk.Entry(frame, textvariable=var, width=30)
                    if readonly:
                        entry.config(state="readonly")
                    entry.grid(row=row_idx, column=1, padx=5, pady=5, sticky="w")
                    
                    desc_or_unit = param.get("unit", "")
                    if p_desc:
                        desc_or_unit = p_desc
                    if desc_or_unit:
                        ttk.Label(frame, text=desc_or_unit).grid(row=row_idx, column=2, padx=5, pady=5, sticky="w")
                        
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
                    
                # Define a fixed column order for readability
                preferred_order = ["id", "name", "MemStartAddress", "MemEraseSectorSize",
                                   "MemNumberOfSectors", "MemMinReadSize", "MemWritePageSize",
                                   "MemSpecifiedEraseCycles"]
                ordered_headers = [h for h in preferred_order if h in headers]
                remaining = sorted([h for h in headers if h not in preferred_order])
                headers = ordered_headers + remaining
                
                tree_frame = ttk.Frame(frame)
                tree_frame.grid(row=row_idx, column=0, columnspan=3, padx=5, pady=5, sticky="we")
                
                ttk.Label(tree_frame, text=l_name).pack(anchor="w")
                
                tree = ttk.Treeview(tree_frame, columns=headers, show="headings", height=8, selectmode="browse")
                for h in headers:
                    tree.heading(h, text=h)
                    col_width = 140 if h in ("MemStartAddress", "MemEraseSectorSize") else 100
                    tree.column(h, width=col_width, anchor="center")
                    
                for item_dict in items_data:
                    row_vals = [item_dict.get(h, "") for h in headers]
                    tree.insert("", "end", values=row_vals)
                    
                # Add horizontal scrollbar for wide table
                h_scrollbar = ttk.Scrollbar(tree_frame, orient="horizontal", command=tree.xview)
                tree.configure(xscrollcommand=h_scrollbar.set)
                tree.pack(fill="x", expand=True)
                h_scrollbar.pack(fill="x")
                
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
        edit_win.geometry("450x500")
        edit_win.grab_set()
        
        # Create scrollable frame for many fields
        canvas = tk.Canvas(edit_win)
        scrollbar = ttk.Scrollbar(edit_win, orient="vertical", command=canvas.yview)
        scroll_frame = ttk.Frame(canvas)
        scroll_frame.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=scroll_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
        entries = {}
        for idx, h in enumerate(headers):
            ttk.Label(scroll_frame, text=h + ":").grid(row=idx, column=0, padx=10, pady=5, sticky="e")
            # Auto-fill id based on current tree item count
            default_val = ""
            if h == "id":
                default_val = str(len(tree.get_children()))
            var = tk.StringVar(value=default_val)
            ttk.Entry(scroll_frame, textvariable=var, width=30).grid(row=idx, column=1, padx=10, pady=5, sticky="w")
            entries[h] = var
            
        def save_new():
            new_vals = [entries[h].get() for h in headers]
            tree.insert("", "end", values=new_vals)
            edit_win.destroy()
            
        ttk.Button(scroll_frame, text="Add", command=save_new).grid(row=len(headers), column=0, columnspan=2, pady=15)

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
        edit_win.geometry("450x500")
        edit_win.grab_set()
        
        # Create scrollable frame for many fields
        canvas = tk.Canvas(edit_win)
        scrollbar = ttk.Scrollbar(edit_win, orient="vertical", command=canvas.yview)
        scroll_frame = ttk.Frame(canvas)
        scroll_frame.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=scroll_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
        entries = {}
        for idx, h in enumerate(headers):
            ttk.Label(scroll_frame, text=h + ":").grid(row=idx, column=0, padx=10, pady=5, sticky="e")
            val = current_vals[idx] if idx < len(current_vals) else ""
            var = tk.StringVar(value=val)
            ttk.Entry(scroll_frame, textvariable=var, width=30).grid(row=idx, column=1, padx=10, pady=5, sticky="w")
            entries[h] = var
            
        def save_edit():
            new_vals = [entries[h].get() for h in headers]
            tree.item(item_id, values=new_vals)
            edit_win.destroy()
            
        ttk.Button(scroll_frame, text="Save", command=save_edit).grid(row=len(headers), column=0, columnspan=2, pady=15)

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

    def _group_sectors_into_batches(self, sector_list):
        """
        Nhóm các sector liên tiếp có cùng kích thước (MemEraseSectorSize) thành 1 batch.
        
        VD: 4 sector 16KB liên tiếp -> 1 batch với MemNumberOfSectors=4
            1 sector 64KB           -> 1 batch với MemNumberOfSectors=1
            3 sector 128KB liên tiếp -> 1 batch với MemNumberOfSectors=3
            
        Trả về list các dict đã nhóm, sẵn sàng cho Jinja2 template.
        """
        if not sector_list:
            return []
        
        # Sắp xếp theo StartAddress tăng dần
        sorted_sectors = sorted(sector_list, key=lambda s: int(str(s.get("MemStartAddress", "0")), 0))
        
        batches = []
        current_batch = None
        
        for sector in sorted_sectors:
            erase_size = int(str(sector.get("MemEraseSectorSize", "0")))
            start_addr = str(sector.get("MemStartAddress", "0"))
            start_addr_int = int(start_addr, 0)
            min_read = str(sector.get("MemMinReadSize", "1"))
            write_page = str(sector.get("MemWritePageSize", "1"))
            erase_cycles = str(sector.get("MemSpecifiedEraseCycles", "10000"))
            
            # Kiểm tra xem sector có liên tiếp và cùng kích thước với batch hiện tại không
            need_new_batch = True
            if current_batch is not None and current_batch["_erase_size"] == erase_size:
                # Kiểm tra tính liên tục về địa chỉ
                expected_next = int(current_batch["MemStartAddress"], 0) + \
                                current_batch["MemNumberOfSectors"] * current_batch["_erase_size"]
                if start_addr_int == expected_next:
                    need_new_batch = False
            
            if need_new_batch:
                # Bắt đầu batch mới
                current_batch = {
                    "MemStartAddress": start_addr,
                    "MemNumberOfSectors": 1,
                    "MemEraseSectorSize": str(erase_size),
                    "MemMinReadSize": min_read,
                    "MemWritePageSize": write_page,
                    "MemSpecifiedEraseCycles": erase_cycles,
                    "_erase_size": erase_size,
                    "_sector_names": [sector.get("name", "")]
                }
                batches.append(current_batch)
            else:
                # Cộng thêm sector vào batch hiện tại
                current_batch["MemNumberOfSectors"] += 1
                current_batch["_sector_names"].append(sector.get("name", ""))
        
        # Tạo comment cho mỗi batch
        for i, batch in enumerate(batches):
            sector_names = batch.pop("_sector_names")
            batch.pop("_erase_size")
            
            num_sectors = batch["MemNumberOfSectors"]
            erase_size = int(batch["MemEraseSectorSize"])
            
            # Tạo size comment (VD: "16 KB", "64 KB", "128 KB")
            if erase_size >= 1024:
                batch["size_comment"] = f"{erase_size // 1024} KB"
            else:
                batch["size_comment"] = f"{erase_size} B"
            
            # Tạo comment mô tả batch
            if num_sectors == 1:
                batch["comment"] = f"Nhóm {i+1}: {sector_names[0]}"
            else:
                first_name = sector_names[0]
                last_name = sector_names[-1]
                # Extract sector numbers from names like "SECTOR_0"
                try:
                    first_num = first_name.split("_")[-1]
                    last_num = last_name.split("_")[-1]
                    batch["comment"] = f"Nhóm {i+1}: {num_sectors} Sector ({first_name} -> {last_name})"
                except:
                    batch["comment"] = f"Nhóm {i+1}: {num_sectors} Sector(s)"
        
        return batches

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

        # ================================================================
        # Build config dictionary for Jinja2
        # ================================================================
        config = {}
        for c_name, params in self.ui_vars.items():
            config[c_name] = {}
            for p_name, var in params.items():
                val = var.get()
                # Convert boolean values to AUTOSAR STD_ON/STD_OFF
                if val == "true":
                    val = "STD_ON"
                elif val == "false":
                    val = "STD_OFF"
                config[c_name][p_name] = val

        # ================================================================
        # Lấy raw sector list từ Treeview và nhóm thành batches
        # ================================================================
        raw_sectors = []
        for (tc_name, tl_name), (tree, headers) in self.trees.items():
            if tl_name == "MemSectorBatch":
                raw_sectors = self.get_tree_data(tc_name, tl_name)
                # Also store raw list in config for backward compatibility
                config[tl_name] = raw_sectors
        
        sector_batches = self._group_sectors_into_batches(raw_sectors)

        # ================================================================
        # Tạo thư mục output nếu chưa có
        # ================================================================
        if not os.path.exists(OUTPUT_DIR):
            os.makedirs(OUTPUT_DIR)

        try:
            # ============================================================
            # 1. Generate Mem_Cfg.h (từ templates/include)
            # ============================================================
            env_include = Environment(loader=FileSystemLoader(INCLUDE_TEMPLATE_DIR))
            
            template_mem_h = env_include.get_template("Mem_Cfg.h.template")
            mem_h_content = template_mem_h.render(config=config)
            with open(os.path.join(OUTPUT_DIR, "Mem_Cfg.h"), 'w', encoding='utf-8') as f:
                f.write(mem_h_content)

            # ============================================================
            # 2. Generate Flash_IP_Cfg.h (từ templates/include)
            #    Tính toán các giá trị derived từ PSIZE và sector table
            # ============================================================
            # PSIZE -> Write Alignment mapping
            psize_to_alignment = {
                "PSIZE_x8": 1,
                "PSIZE_x16": 2,
                "PSIZE_x32": 4,
                "PSIZE_x64": 8,
            }
            psize_val = config.get("FlashIPConfig", {}).get("FlashPSize", "PSIZE_x32")
            flash_ip_write_alignment = psize_to_alignment.get(psize_val, 4)
            
            # Đếm tổng số sector từ bảng sector
            flash_ip_total_sectors = len(raw_sectors)
            
            template_flash_h = env_include.get_template("Flash_IP_Cfg.h.template")
            flash_h_content = template_flash_h.render(
                config=config,
                flash_ip_write_alignment=flash_ip_write_alignment,
                flash_ip_total_sectors=flash_ip_total_sectors
            )
            with open(os.path.join(OUTPUT_DIR, "Flash_IP_Cfg.h"), 'w', encoding='utf-8') as f:
                f.write(flash_h_content)

            # ============================================================
            # 3. Generate Mem_Cfg.c (từ templates/src)
            # ============================================================
            env_src = Environment(loader=FileSystemLoader(SRC_TEMPLATE_DIR))
            
            template_mem_c = env_src.get_template("Mem_Cfg.c.template")
            mem_c_content = template_mem_c.render(
                config=config,
                sector_batches=sector_batches
            )
            with open(os.path.join(OUTPUT_DIR, "Mem_Cfg.c"), 'w', encoding='utf-8') as f:
                f.write(mem_c_content)

            # ============================================================
            # 4. Generate Flash_IP_Cfg.c (chỉ include header, không cần template)
            # ============================================================
            with open(os.path.join(OUTPUT_DIR, "Flash_IP_Cfg.c"), 'w', encoding='utf-8') as f:
                f.write('#include "Flash_IP_Cfg.h"\n')

            # ============================================================
            # Thông báo thành công
            # ============================================================
            generated_files = ["Mem_Cfg.h", "Mem_Cfg.c", "Flash_IP_Cfg.h", "Flash_IP_Cfg.c"]
            file_list = "\n".join([f"  ✓ {f}" for f in generated_files])
            messagebox.showinfo(
                "Thành công", 
                f"Đã generate {len(generated_files)} file thành công tại:\n{OUTPUT_DIR}\n\n{file_list}"
            )
        except Exception as e:
            messagebox.showerror("Lỗi Generate", f"Lỗi trong quá trình generate:\n{e}")

if __name__ == "__main__":
    root = tk.Tk()
    app = MemConfiguratorApp(root)
    root.mainloop()
