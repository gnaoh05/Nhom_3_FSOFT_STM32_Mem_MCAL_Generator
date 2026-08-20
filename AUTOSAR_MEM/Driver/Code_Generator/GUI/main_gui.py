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


class ToolTip:
    """Lightweight tooltip that appears on hover, similar to AUTOSAR configuration tools."""
    def __init__(self, widget, text):
        self.widget = widget
        self.text = text
        self.tip_window = None
        widget.bind("<Enter>", self._show)
        widget.bind("<Leave>", self._hide)

    def _show(self, event=None):
        if self.tip_window or not self.text:
            return
        x = self.widget.winfo_rootx() + 20
        y = self.widget.winfo_rooty() + self.widget.winfo_height() + 2
        self.tip_window = tw = tk.Toplevel(self.widget)
        tw.wm_overrideredirect(True)
        tw.wm_geometry(f"+{x}+{y}")
        label = tk.Label(tw, text=self.text, justify="left",
                         background="#ffffe0", relief="solid", borderwidth=1,
                         font=("Segoe UI", 9))
        label.pack(ipadx=4, ipady=2)

    def _hide(self, event=None):
        if self.tip_window:
            self.tip_window.destroy()
            self.tip_window = None

class MemConfiguratorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("AUTOSAR Mem Driver Configurator (EPD/EPC)")
        self.root.geometry("1050x900")
        
        self.epd_tree = None
        self.ui_vars = {} # Maps container_name -> { param_name: tk.Variable }
        self.list_data = {} # Maps container_name -> { list_name: [ dicts ] }
        self.trees = {} # Maps (c_name, l_name) -> (tree_widget, headers_list)
        # Store parameter metadata: { (container_name, param_name): { "type", "min", "max", "desc" } }
        self.param_meta = {}
        # Store sub-container (sector batch) parameter metadata for validation
        # { param_name: { "type", "min", "max", "desc" } }
        self.sector_param_meta = {}
        
        self.load_epd(EPD_FILE)
        self.setup_ui()

    def _parse_arxml_to_custom_tree(self, epd_path):
        """
        Parse AUTOSAR ARXML (.epd) file and convert to internal custom XML tree.
        Supports:
          - ECUC-BOOLEAN-PARAM-DEF, ECUC-INTEGER-PARAM-DEF, ECUC-FLOAT-PARAM-DEF, ECUC-ENUMERATION-PARAM-DEF
          - MIN/MAX extraction for INTEGER and FLOAT params
          - SUB-CONTAINERS (e.g., MemSectorBatch nested inside MemInstance)
          - DESC extraction for parameter descriptions
        """
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
        
        # Process only top-level containers (direct children of CONTAINERS)
        containers_elem = module_def.find('ns:CONTAINERS', ns)
        if containers_elem is None:
            return ET.ElementTree(custom_root)
        
        for container in containers_elem.findall('ns:ECUC-PARAM-CONF-CONTAINER-DEF', ns):
            c_name_elem = container.find('ns:SHORT-NAME', ns)
            if c_name_elem is None:
                continue
            c_name = c_name_elem.text
            
            # Extract container description
            c_desc = ""
            c_desc_elem = container.find('ns:DESC/ns:L-2', ns)
            if c_desc_elem is not None and c_desc_elem.text:
                c_desc = c_desc_elem.text.strip()
            
            custom_cont = ET.SubElement(custom_mod, "Container", name=c_name, description=c_desc)
            
            # Process PARAMETERS of this container
            params_elem = container.find('ns:PARAMETERS', ns)
            if params_elem is not None:
                self._parse_params(params_elem, custom_cont, c_name, ns)
            
            # Process SUB-CONTAINERS (e.g., MemSectorBatch inside MemInstance)
            sub_containers_elem = container.find('ns:SUB-CONTAINERS', ns)
            if sub_containers_elem is not None:
                for sub_cont in sub_containers_elem.findall('ns:ECUC-PARAM-CONF-CONTAINER-DEF', ns):
                    sc_name_elem = sub_cont.find('ns:SHORT-NAME', ns)
                    if sc_name_elem is None:
                        continue
                    sc_name = sc_name_elem.text
                    
                    # Parse sub-container parameters to build List definition with headers
                    sc_params_elem = sub_cont.find('ns:PARAMETERS', ns)
                    if sc_params_elem is not None:
                        # Build list element with parameter definitions as headers
                        list_elem = ET.SubElement(custom_cont, "List", name=sc_name)
                        
                        for param in sc_params_elem:
                            tag = param.tag.replace('{http://autosar.org/schema/r4.0}', '')
                            if not tag.endswith('-PARAM-DEF'):
                                continue
                            
                            p_name_elem = param.find('ns:SHORT-NAME', ns)
                            if p_name_elem is None:
                                continue
                            p_name = p_name_elem.text
                            
                            p_type, default_val, p_min, p_max, p_desc, options = \
                                self._extract_param_info(param, tag, ns)
                            
                            # Store sector param metadata for validation
                            self.sector_param_meta[p_name] = {
                                "type": p_type, "min": p_min, "max": p_max, "desc": p_desc
                            }
                            
                            # Add column definition to List
                            col_attribs = {"name": p_name, "type": p_type, "default": default_val}
                            if p_min:
                                col_attribs["min"] = p_min
                            if p_max:
                                col_attribs["max"] = p_max
                            ET.SubElement(list_elem, "Column", **col_attribs)
                        
                        # Add default sector items for STM32F401RE
                        default_sectors = [
                            {"id": "0", "name": "SECTOR_0", "MemNumberOfSectors": "1", "MemEraseSectorSize": "16384",  "MemStartAddress": "0x08000000", "MemMinReadSize": "1", "MemWritePageSize": "1", "MemSpecifiedEraseCycles": "10000"},
                            {"id": "1", "name": "SECTOR_1", "MemNumberOfSectors": "1", "MemEraseSectorSize": "16384",  "MemStartAddress": "0x08004000", "MemMinReadSize": "1", "MemWritePageSize": "1", "MemSpecifiedEraseCycles": "10000"},
                            {"id": "2", "name": "SECTOR_2", "MemNumberOfSectors": "1", "MemEraseSectorSize": "16384",  "MemStartAddress": "0x08008000", "MemMinReadSize": "1", "MemWritePageSize": "1", "MemSpecifiedEraseCycles": "10000"},
                            {"id": "3", "name": "SECTOR_3", "MemNumberOfSectors": "1", "MemEraseSectorSize": "16384",  "MemStartAddress": "0x0800C000", "MemMinReadSize": "1", "MemWritePageSize": "1", "MemSpecifiedEraseCycles": "10000"},
                            {"id": "4", "name": "SECTOR_4", "MemNumberOfSectors": "1", "MemEraseSectorSize": "65536",  "MemStartAddress": "0x08010000", "MemMinReadSize": "1", "MemWritePageSize": "1", "MemSpecifiedEraseCycles": "10000"},
                            {"id": "5", "name": "SECTOR_5", "MemNumberOfSectors": "1", "MemEraseSectorSize": "131072", "MemStartAddress": "0x08020000", "MemMinReadSize": "1", "MemWritePageSize": "1", "MemSpecifiedEraseCycles": "10000"},
                            {"id": "6", "name": "SECTOR_6", "MemNumberOfSectors": "1", "MemEraseSectorSize": "131072", "MemStartAddress": "0x08040000", "MemMinReadSize": "1", "MemWritePageSize": "1", "MemSpecifiedEraseCycles": "10000"},
                            {"id": "7", "name": "SECTOR_7", "MemNumberOfSectors": "1", "MemEraseSectorSize": "131072", "MemStartAddress": "0x08060000", "MemMinReadSize": "1", "MemWritePageSize": "1", "MemSpecifiedEraseCycles": "10000"},
                        ]
                        for sector in default_sectors:
                            ET.SubElement(list_elem, "Item", **sector)
                    
        return ET.ElementTree(custom_root)

    def _extract_param_info(self, param, tag, ns):
        """Extract type, default, min, max, description, and enum options from an ARXML param element."""
        p_type = "STRING"
        if "BOOLEAN" in tag: p_type = "BOOLEAN"
        elif "INTEGER" in tag: p_type = "INTEGER"
        elif "FLOAT" in tag: p_type = "FLOAT"
        elif "ENUMERATION" in tag: p_type = "ENUM"
        
        # Default value
        default_val = ""
        def_elem = param.find('ns:DEFAULT-VALUE', ns)
        if def_elem is not None and def_elem.text:
            default_val = def_elem.text
        
        # MIN/MAX for INTEGER and FLOAT
        p_min = ""
        p_max = ""
        if p_type in ("INTEGER", "FLOAT"):
            min_elem = param.find('ns:MIN', ns)
            if min_elem is not None and min_elem.text:
                p_min = min_elem.text
            max_elem = param.find('ns:MAX', ns)
            if max_elem is not None and max_elem.text:
                p_max = max_elem.text
        
        # Description
        p_desc = ""
        desc_elem = param.find('ns:DESC/ns:L-2', ns)
        if desc_elem is not None and desc_elem.text:
            p_desc = desc_elem.text.strip()
        
        # Enum options
        options = []
        if p_type == "ENUM":
            for lit in param.findall('.//ns:ECUC-ENUMERATION-LITERAL-DEF/ns:SHORT-NAME', ns):
                if lit.text:
                    options.append(lit.text)
        
        return p_type, default_val, p_min, p_max, p_desc, options

    def _parse_params(self, params_elem, custom_cont, c_name, ns):
        """Parse PARAMETERS element and add Parameter sub-elements to custom_cont."""
        for param in params_elem:
            tag = param.tag.replace('{http://autosar.org/schema/r4.0}', '')
            if not tag.endswith('-PARAM-DEF'):
                continue
            
            p_name_elem = param.find('ns:SHORT-NAME', ns)
            if p_name_elem is None:
                continue
            p_name = p_name_elem.text
            
            p_type, default_val, p_min, p_max, p_desc, options = \
                self._extract_param_info(param, tag, ns)
            
            # Build attributes
            attribs = {"name": p_name, "type": p_type, "default": default_val}
            if p_desc:
                attribs["description"] = p_desc
            if options:
                attribs["options"] = ",".join(options)
            if p_min:
                attribs["min"] = p_min
            if p_max:
                attribs["max"] = p_max
            
            ET.SubElement(custom_cont, "Parameter", **attribs)
            
            # Store metadata for validation
            self.param_meta[(c_name, p_name)] = {
                "type": p_type, "min": p_min, "max": p_max, "desc": p_desc
            }

    def load_epd(self, epd_path):
        if not os.path.exists(epd_path):
            messagebox.showerror("Error", f"EPD file not found at:\n{epd_path}")
            return
        try:
            temp_tree = ET.parse(epd_path)
            root_tag = temp_tree.getroot().tag
            
            if root_tag.endswith("AUTOSAR") or 'autosar.org' in root_tag:
                # Parse as ARXML
                self.epd_tree = self._parse_arxml_to_custom_tree(epd_path)
            else:
                # Parse as Custom XML (AUTOSAR_EPD)
                self.epd_tree = temp_tree
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load EPD file:\n{e}")

    def _validate_value(self, value_str, p_type, p_min, p_max, p_name):
        """
        Validate a parameter value against its type and range.
        Returns (is_valid, error_message).
        """
        if p_type == "INTEGER":
            try:
                val = int(str(value_str), 0)  # Support hex (0x...) and decimal
            except ValueError:
                return False, f"'{p_name}': Giá trị '{value_str}' không phải số nguyên hợp lệ."
            
            if p_min:
                min_val = int(p_min, 0) if p_min.startswith("0x") or p_min.startswith("0X") else int(p_min)
                if val < min_val:
                    return False, f"'{p_name}': Giá trị {val} nhỏ hơn MIN={min_val}."
            if p_max:
                max_val = int(p_max, 0) if p_max.startswith("0x") or p_max.startswith("0X") else int(p_max)
                if val > max_val:
                    return False, f"'{p_name}': Giá trị {val} lớn hơn MAX={max_val}."
        
        elif p_type == "FLOAT":
            try:
                val = float(value_str)
            except ValueError:
                return False, f"'{p_name}': Giá trị '{value_str}' không phải số thực hợp lệ."
            
            if p_min:
                if val < float(p_min):
                    return False, f"'{p_name}': Giá trị {val} nhỏ hơn MIN={p_min}."
            if p_max:
                if val > float(p_max):
                    return False, f"'{p_name}': Giá trị {val} lớn hơn MAX={p_max}."
        
        return True, ""

    def _get_range_hint(self, p_type, p_min, p_max):
        """Build a range hint string for display, e.g. '[1..4294967295]'."""
        if p_type in ("INTEGER", "FLOAT") and (p_min or p_max):
            min_str = p_min if p_min else "..."
            max_str = p_max if p_max else "..."
            return f"[{min_str}..{max_str}]"
        return ""
            
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
        
        frame_id = canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        
        def _configure_canvas(event):
            canvas.itemconfig(frame_id, width=event.width)
            
        canvas.bind("<Configure>", _configure_canvas)
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
                p_min = param.get("min", "")
                p_max = param.get("max", "")
                
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
                    # INTEGER, FLOAT, STRING, HEX
                    var = tk.StringVar(value=p_default)
                    readonly = param.get("readonly") == "true"
                    entry = ttk.Entry(frame, textvariable=var, width=30)
                    if readonly:
                        entry.config(state="readonly")
                    entry.grid(row=row_idx, column=1, padx=5, pady=5, sticky="w")
                    
                    # Show only description as label (no range clutter)
                    if p_desc:
                        ttk.Label(frame, text=p_desc).grid(row=row_idx, column=2, padx=5, pady=5, sticky="w")
                    
                    # Tooltip shows range info on hover (like professional AUTOSAR tools)
                    range_hint = self._get_range_hint(p_type, p_min, p_max)
                    if range_hint:
                        ToolTip(entry, f"{p_name}\nRange: {range_hint}")
                    
                    # Add FocusOut validation for INTEGER/FLOAT parameters
                    if p_type in ("INTEGER", "FLOAT") and not readonly:
                        def _make_validator(v, pt, pmin, pmax, pn, c=c_name):
                            def _on_focus_out(event):
                                val_str = v.get()
                                if not val_str:
                                    return
                                is_valid, err_msg = self._validate_value(val_str, pt, pmin, pmax, pn)
                                if not is_valid:
                                    messagebox.showwarning("Validation Error", err_msg)
                            return _on_focus_out
                        entry.bind("<FocusOut>", _make_validator(var, p_type, p_min, p_max, p_name))
                        
                    self.ui_vars[c_name][p_name] = var
                    
                row_idx += 1
                
            # Lists (sub-containers converted to lists)
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

    def _validate_sector_item(self, data_dict):
        """
        Validate all fields in a sector batch item against AUTOSAR ranges.
        Returns (is_valid, list_of_errors).
        """
        errors = []
        for p_name, val_str in data_dict.items():
            if p_name in ("id", "name"):
                continue
            if p_name in self.sector_param_meta:
                meta = self.sector_param_meta[p_name]
                p_type = meta.get("type", "STRING")
                p_min = meta.get("min", "")
                p_max = meta.get("max", "")
                
                if val_str and p_type in ("INTEGER", "FLOAT"):
                    is_valid, err_msg = self._validate_value(val_str, p_type, p_min, p_max, p_name)
                    if not is_valid:
                        errors.append(err_msg)
        return len(errors) == 0, errors

    def add_tree_item(self, c_name, l_name):
        tree, headers = self.trees[(c_name, l_name)]
        
        edit_win = tk.Toplevel(self.root)
        edit_win.title("Add New Item")
        edit_win.geometry("550x600")
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
            entry = ttk.Entry(scroll_frame, textvariable=var, width=30)
            entry.grid(row=idx, column=1, padx=10, pady=5, sticky="w")
            entries[h] = var
            
            # Tooltip with range info on hover
            if h in self.sector_param_meta:
                meta = self.sector_param_meta[h]
                range_hint = self._get_range_hint(meta.get("type", ""), meta.get("min", ""), meta.get("max", ""))
                if range_hint:
                    ToolTip(entry, f"{h}\nRange: {range_hint}")
            
        def save_new():
            # Validate before saving
            data = {h: entries[h].get() for h in headers}
            is_valid, errors = self._validate_sector_item(data)
            if not is_valid:
                messagebox.showerror("Validation Error", 
                    "Giá trị không hợp lệ theo AUTOSAR range:\n\n" + "\n".join(errors))
                return
            
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
        edit_win.geometry("550x600")
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
            entry = ttk.Entry(scroll_frame, textvariable=var, width=30)
            entry.grid(row=idx, column=1, padx=10, pady=5, sticky="w")
            entries[h] = var
            
            # Tooltip with range info on hover
            if h in self.sector_param_meta:
                meta = self.sector_param_meta[h]
                range_hint = self._get_range_hint(meta.get("type", ""), meta.get("min", ""), meta.get("max", ""))
                if range_hint:
                    ToolTip(entry, f"{h}\nRange: {range_hint}")
            
        def save_edit():
            # Validate before saving
            data = {h: entries[h].get() for h in headers}
            is_valid, errors = self._validate_sector_item(data)
            if not is_valid:
                messagebox.showerror("Validation Error", 
                    "Giá trị không hợp lệ theo AUTOSAR range:\n\n" + "\n".join(errors))
                return
            
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

    def _validate_all_params(self):
        """
        Validate all parameter values in the UI against their AUTOSAR ranges.
        Returns (is_valid, list_of_errors).
        """
        errors = []
        for (c_name, p_name), meta in self.param_meta.items():
            if c_name not in self.ui_vars or p_name not in self.ui_vars[c_name]:
                continue
            val_str = self.ui_vars[c_name][p_name].get()
            if not val_str:
                continue
            
            p_type = meta.get("type", "STRING")
            p_min = meta.get("min", "")
            p_max = meta.get("max", "")
            
            if p_type in ("INTEGER", "FLOAT"):
                is_valid, err_msg = self._validate_value(val_str, p_type, p_min, p_max, f"{c_name}/{p_name}")
                if not is_valid:
                    errors.append(err_msg)
        
        return len(errors) == 0, errors

    def save_epc(self):
        filepath = filedialog.asksaveasfilename(initialdir=BASE_DIR, initialfile="MemDriver.epc", defaultextension=".epc", filetypes=[("EPC XML Files", "*.epc"), ("All Files", "*.*")])
        if not filepath:
            return
        
        # Validate all params before saving
        is_valid, errors = self._validate_all_params()
        if not is_valid:
            result = messagebox.askyesno("Validation Warning", 
                "Một số giá trị ngoài AUTOSAR range:\n\n" + "\n".join(errors) + "\n\nBạn vẫn muốn lưu?")
            if not result:
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
        # Validate all parameters before generating code
        # ================================================================
        is_valid, errors = self._validate_all_params()
        if not is_valid:
            messagebox.showerror("Validation Error", 
                "Không thể generate code - giá trị ngoài AUTOSAR range:\n\n" + "\n".join(errors))
            return

        # Validate sector table items too
        for (tc_name, tl_name), (tree_widget, headers) in self.trees.items():
            if tl_name == "MemSectorBatch":
                sector_data = self.get_tree_data(tc_name, tl_name)
                for i, sector in enumerate(sector_data):
                    s_valid, s_errors = self._validate_sector_item(sector)
                    if not s_valid:
                        sector_name = sector.get("name", f"Row {i}")
                        messagebox.showerror("Validation Error",
                            f"Sector '{sector_name}' có giá trị ngoài AUTOSAR range:\n\n" + "\n".join(s_errors))
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
