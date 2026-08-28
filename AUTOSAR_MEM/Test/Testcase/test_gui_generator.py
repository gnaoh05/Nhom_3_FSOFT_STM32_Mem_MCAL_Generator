#!/usr/bin/env python3
# ==============================================================================
# AUTOSAR MEM CONFIGURATOR & GENERATOR TEST SUITE
# Traceability: AUTOSAR_MEM/Test/Core/GUI_Test_Traceability.md
# ==============================================================================

import os
import sys
import time
import unittest
import tempfile
import xml.etree.ElementTree as ET
import subprocess
import shutil

# Force immediate stdout flush for real-time console rendering
sys.stdout.reconfigure(line_buffering=True, encoding='utf-8') if hasattr(sys.stdout, 'reconfigure') else None

# Add GUI directory to python path
TEST_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(TEST_DIR, "../../.."))
GUI_DIR = os.path.join(PROJECT_ROOT, "AUTOSAR_MEM/Driver/Code_Generator/GUI")
sys.path.insert(0, GUI_DIR)

from main_gui import MemConfiguratorApp

try:
    from jinja2 import Environment, FileSystemLoader
    JINJA_AVAILABLE = True
except ImportError:
    JINJA_AVAILABLE = False


class TestMemConfiguratorAndGenerator(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.epd_path = os.path.join(GUI_DIR, "MemDriver.epd")
        cls.epc_path = os.path.join(GUI_DIR, "MemDriver.epc")
        cls.inc_tmpl_dir = os.path.join(PROJECT_ROOT, "AUTOSAR_MEM/Driver/Code_Generator/templates/include")
        cls.src_tmpl_dir = os.path.join(PROJECT_ROOT, "AUTOSAR_MEM/Driver/Code_Generator/templates/src")
        
        # Instantiate a lightweight headless app instance
        cls.app = MemConfiguratorApp.__new__(MemConfiguratorApp)
        cls.app.param_meta = {}
        cls.app.sector_param_meta = {}
        cls.app.trees = {}
        cls.app.ui_vars = {}
        cls.app.list_data = {}
        
        # Parse EPD tree
        cls.epd_tree = cls.app._parse_arxml_to_custom_tree(cls.epd_path)

    # --------------------------------------------------------------------------
    # GROUP 1: EPD ARXML Schema Parsing Tests
    # --------------------------------------------------------------------------
    def test_01_parse_epd_root_module(self):
        """[GUI_EPD_001] Verify parsing of ECUC-MODULE-DEF root tag and module name."""
        self.assertIsNotNone(self.epd_tree)
        root = self.epd_tree.getroot()
        self.assertEqual(root.tag, "AUTOSAR_EPD")
        module = root.find("Module")
        self.assertIsNotNone(module)
        self.assertEqual(module.get("name"), "Mem")

    def test_02_extract_container_definitions(self):
        """[GUI_EPD_002] Verify extraction of top-level and sub-containers."""
        root = self.epd_tree.getroot()
        module = root.find("Module")
        containers = [c.get("name") for c in module.findall("Container")]
        self.assertIn("MemGeneral", containers)
        self.assertIn("MemInstance", containers)
        self.assertIn("FlashIPConfig", containers)

    def test_03_extract_parameter_metadata(self):
        """[GUI_EPD_003] Verify parameter metadata (types, defaults, min/max)."""
        self.assertTrue(len(self.app.param_meta) > 0)
        self.assertTrue(len(self.app.sector_param_meta) > 0)
        
        # Check MemDevErrorDetect boolean param
        meta_dev_err = self.app.param_meta.get(("MemGeneral", "MemDevErrorDetect"))
        self.assertIsNotNone(meta_dev_err)
        self.assertEqual(meta_dev_err["type"], "BOOLEAN")
        
        # Check sector batch integer param
        self.assertIn("MemEraseSectorSize", self.app.sector_param_meta)
        self.assertEqual(self.app.sector_param_meta["MemEraseSectorSize"]["type"], "INTEGER")

    # --------------------------------------------------------------------------
    # GROUP 2: Input Validation & Constraints Tests
    # --------------------------------------------------------------------------
    def test_04_integer_range_validation(self):
        """[GUI_VAL_001] Test Integer min/max validation."""
        # Value within range [1, 10]
        is_valid, msg = self.app._validate_value("5", "INTEGER", "1", "10", "TestParam")
        self.assertTrue(is_valid)
        self.assertEqual(msg, "")
        
        # Value below min
        is_valid, msg = self.app._validate_value("0", "INTEGER", "1", "10", "TestParam")
        self.assertFalse(is_valid)
        self.assertTrue("MIN=1" in msg)
        
        # Value above max
        is_valid, msg = self.app._validate_value("11", "INTEGER", "1", "10", "TestParam")
        self.assertFalse(is_valid)
        self.assertTrue("MAX=10" in msg)

    def test_05_non_numeric_rejection(self):
        """[GUI_VAL_002] Test rejection of non-numeric strings for integer types."""
        is_valid, msg = self.app._validate_value("abc", "INTEGER", "1", "10", "TestParam")
        self.assertFalse(is_valid)
        self.assertTrue("không phải số nguyên" in msg or "TestParam" in msg)

    def test_06_hex_address_format_validation(self):
        """[GUI_VAL_003] Test parsing and validation of Hexadecimal addresses."""
        # Valid hex
        is_valid, msg = self.app._validate_value("0x08000000", "INTEGER", "0", "0xFFFFFFFF", "MemStartAddress")
        self.assertTrue(is_valid)
        
        # Invalid hex
        is_valid, msg = self.app._validate_value("0xNOT_HEX", "INTEGER", "0", "0xFFFFFFFF", "MemStartAddress")
        self.assertFalse(is_valid)

    def test_07_sector_parameter_validation(self):
        """[GUI_VAL_004] Test validation of full sector row dictionary."""
        valid_sector = {
            "id": "0",
            "name": "SECTOR_0",
            "MemNumberOfSectors": "1",
            "MemEraseSectorSize": "16384",
            "MemStartAddress": "0x08000000",
            "MemMinReadSize": "1",
            "MemWritePageSize": "4",
            "MemSpecifiedEraseCycles": "10000"
        }
        is_valid, errors = self.app._validate_sector_item(valid_sector)
        self.assertTrue(is_valid)
        self.assertEqual(len(errors), 0)

    # --------------------------------------------------------------------------
    # GROUP 3: Sector Batch Grouping Algorithm Tests
    # --------------------------------------------------------------------------
    def test_08_group_contiguous_sectors(self):
        """[GUI_GRP_001] Test grouping 4 contiguous 16KB sectors into 1 batch."""
        sectors = [
            {"name": "SECTOR_0", "MemStartAddress": "0x08000000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_1", "MemStartAddress": "0x08004000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_2", "MemStartAddress": "0x08008000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_3", "MemStartAddress": "0x0800C000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
        ]
        batches = self.app._group_sectors_into_batches(sectors)
        self.assertEqual(len(batches), 1)
        self.assertEqual(batches[0]["MemNumberOfSectors"], 4)
        self.assertEqual(batches[0]["MemEraseSectorSize"], "16384")
        self.assertEqual(batches[0]["MemStartAddress"], "0x08000000")

    def test_09_split_on_different_size(self):
        """[GUI_GRP_002] Test splitting into separate batch when sector size changes."""
        sectors = [
            {"name": "SECTOR_3", "MemStartAddress": "0x0800C000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_4", "MemStartAddress": "0x08010000", "MemEraseSectorSize": "65536", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
        ]
        batches = self.app._group_sectors_into_batches(sectors)
        self.assertEqual(len(batches), 2)
        self.assertEqual(batches[0]["MemNumberOfSectors"], 1)
        self.assertEqual(batches[0]["MemEraseSectorSize"], "16384")
        self.assertEqual(batches[1]["MemNumberOfSectors"], 1)
        self.assertEqual(batches[1]["MemEraseSectorSize"], "65536")

    def test_10_full_stm32f401_grouping(self):
        """[GUI_GRP_003] Test full STM32F401RE 8-sector configuration produces exactly 3 batches."""
        sectors = [
            {"name": "SECTOR_0", "MemStartAddress": "0x08000000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_1", "MemStartAddress": "0x08004000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_2", "MemStartAddress": "0x08008000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_3", "MemStartAddress": "0x0800C000", "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_4", "MemStartAddress": "0x08010000", "MemEraseSectorSize": "65536", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_5", "MemStartAddress": "0x08020000", "MemEraseSectorSize": "131072", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_6", "MemStartAddress": "0x08040000", "MemEraseSectorSize": "131072", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
            {"name": "SECTOR_7", "MemStartAddress": "0x08060000", "MemEraseSectorSize": "131072", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000"},
        ]
        batches = self.app._group_sectors_into_batches(sectors)
        self.assertEqual(len(batches), 3)
        self.assertEqual(batches[0]["MemNumberOfSectors"], 4)  # 4 x 16KB
        self.assertEqual(batches[1]["MemNumberOfSectors"], 1)  # 1 x 64KB
        self.assertEqual(batches[2]["MemNumberOfSectors"], 3)  # 3 x 128KB

    # --------------------------------------------------------------------------
    # GROUP 4: EPC File Save & Load Round-Trip Tests
    # --------------------------------------------------------------------------
    def test_11_save_epc_structure(self):
        """[GUI_EPC_001] Test EPC XML tree generation and serialization."""
        root = ET.Element("AUTOSAR_EPC")
        module = ET.SubElement(root, "Module", name="Mem")
        cont = ET.SubElement(module, "Container", name="MemGeneral")
        ET.SubElement(cont, "Parameter", name="MemDevErrorDetect", value="true")
        
        xml_str = ET.tostring(root, encoding="utf-8").decode("utf-8")
        self.assertIn("<AUTOSAR_EPC>", xml_str)
        self.assertIn("MemDevErrorDetect", xml_str)

    def test_12_load_epc_data_integrity(self):
        """[GUI_EPC_002] Test loading an existing EPC file and verifying elements."""
        self.assertTrue(os.path.exists(self.epc_path))
        tree = ET.parse(self.epc_path)
        root = tree.getroot()
        module = root.find("Module")
        self.assertIsNotNone(module)
        
        containers = {c.get("name"): c for c in module.findall("Container")}
        self.assertIn("MemGeneral", containers)
        self.assertIn("MemInstance", containers)

    def test_13_corrupted_epc_handling(self):
        """[GUI_EPC_003] Test error handling on malformed EPC XML content."""
        with tempfile.NamedTemporaryFile('w', delete=False, suffix=".epc") as f:
            f.write("<INVALID_XML_UNCLOSED_TAG>")
            temp_path = f.name
        
        try:
            with self.assertRaises(ET.ParseError):
                ET.parse(temp_path)
        finally:
            if os.path.exists(temp_path):
                os.remove(temp_path)

    # --------------------------------------------------------------------------
    # GROUP 5: Jinja2 Template Code Generation Tests
    # --------------------------------------------------------------------------
    def test_14_generate_mem_cfg_h(self):
        """[GUI_GEN_001] Test rendering Mem_Cfg.h template."""
        self.assertTrue(JINJA_AVAILABLE, "Jinja2 library must be installed.")
        env = Environment(loader=FileSystemLoader(self.inc_tmpl_dir))
        template = env.get_template("Mem_Cfg.h.template")
        
        config = {
            "MemGeneral": {"MemDevErrorDetect": "STD_ON", "MemIndex": "0"},
            "MemInstance": {"MemNumberOfInstances": "1"}
        }
        output = template.render(config=config)
        self.assertIn("#define MEM_DEV_ERROR_DETECT  (STD_ON)", output)
        self.assertIn("#define MEM_MAX_INSTANCES     (1u)", output)

    def test_15_generate_mem_cfg_c(self):
        """[GUI_GEN_002] Test rendering Mem_Cfg.c template with sector batches."""
        self.assertTrue(JINJA_AVAILABLE)
        env = Environment(loader=FileSystemLoader(self.src_tmpl_dir))
        template = env.get_template("Mem_Cfg.c.template")
        
        sector_batches = [
            {"MemStartAddress": "0x08000000", "MemNumberOfSectors": 4, "MemEraseSectorSize": "16384", "MemMinReadSize": "1", "MemWritePageSize": "4", "MemSpecifiedEraseCycles": "10000", "size_comment": "16 KB", "comment": "Nhóm 1: 4 Sector (SECTOR_0 -> SECTOR_3)"}
        ]
        config = {"MemInstance": {"MemNumberOfInstances": "1"}}
        output = template.render(config=config, sector_batches=sector_batches)
        self.assertIn("Mem_ConfigData", output)
        self.assertIn(".MemEraseSectorSize    = 16384u", output)

    def test_16_generate_flash_ip_cfg(self):
        """[GUI_GEN_003] Test rendering Flash_IP_Cfg.h and Flash_IP_Cfg.c."""
        self.assertTrue(JINJA_AVAILABLE)
        env_inc = Environment(loader=FileSystemLoader(self.inc_tmpl_dir))
        env_src = Environment(loader=FileSystemLoader(self.src_tmpl_dir))
        
        template_h = env_inc.get_template("Flash_IP_Cfg.h.template")
        template_c = env_src.get_template("Flash_IP_Cfg.c.template")
        
        config = {
            "FlashIPConfig": {
                "FlashBaseAddr": "0x08000000",
                "FlashLatency": "FLASH_LATENCY_2_WS",
                "FlashPrefetchEnable": "STD_ON",
                "FlashICacheEnable": "STD_ON",
                "FlashDCacheEnable": "STD_ON",
                "FlashTimeoutValue": "500000",
                "FlashPSize": "PSIZE_x32"
            }
        }
        out_h = template_h.render(config=config, flash_ip_write_alignment=4, flash_ip_total_sectors=8)
        out_c = template_c.render(config=config)
        
        self.assertIn("#define FLASH_IP_BASE_ADDR", out_h)
        self.assertIn("Flash_IP_Config", out_c)

    # --------------------------------------------------------------------------
    # GROUP 6: End-to-End Build Verification Tests
    # --------------------------------------------------------------------------
    def test_17_compile_generated_code_std_on(self):
        """[GUI_E2E_001] Verify GCC compiles Mem.c successfully with generated headers (STD_ON)."""
        gcc_bin = shutil.which("gcc")
        if not gcc_bin:
            self.skipTest("GCC not found in system PATH, skipping compile test.")
            return

        cmd = [
            gcc_bin, "-fsyntax-only",
            os.path.join(PROJECT_ROOT, "AUTOSAR_MEM/Driver/src/Mem.c"),
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Driver/include')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Generated_File')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Stub/std')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Stub/Det')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Stub/MemAcc')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Stub/SchM')}",
            "-Wall", "-Wextra", "-Werror"
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(res.returncode, 0, f"Compilation failed on STD_ON:\n{res.stderr}")

    def test_18_compile_generated_code_std_off(self):
        """[GUI_E2E_002] Verify GCC compiles Mem.c successfully with STD_OFF (0 errors)."""
        gcc_bin = shutil.which("gcc")
        if not gcc_bin:
            self.skipTest("GCC not found in system PATH, skipping compile test.")
            return

        cmd = [
            gcc_bin, "-fsyntax-only",
            os.path.join(PROJECT_ROOT, "AUTOSAR_MEM/Driver/src/Mem.c"),
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Driver/include')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Generated_File')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Stub/std')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Stub/Det')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Stub/MemAcc')}",
            f"-I{os.path.join(PROJECT_ROOT, 'AUTOSAR_MEM/Test/Stub/SchM')}",
            "-include", os.path.join(PROJECT_ROOT, "AUTOSAR_MEM/Test/Stub/std/Std_Types.h"),
            "-D", "MEM_DEV_ERROR_DETECT=(STD_OFF)",
            "-Wall", "-Wextra"
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(res.returncode, 0, f"Compilation failed on STD_OFF:\n{res.stderr}")


# ==============================================================================
# Clean, Streamed, and Instant Test Runner
# ==============================================================================
def run_clean_test_suite():
    tc_meta = {
        "test_01_parse_epd_root_module": ("GUI_EPD_001", "ParseEpdRootModule", "EPD Schema"),
        "test_02_extract_container_definitions": ("GUI_EPD_002", "ExtractContainerDefinitions", "EPD Schema"),
        "test_03_extract_parameter_metadata": ("GUI_EPD_003", "ExtractParameterMetadata", "EPD Schema"),
        "test_04_integer_range_validation": ("GUI_VAL_001", "IntegerRangeValidation", "Validation"),
        "test_05_non_numeric_rejection": ("GUI_VAL_002", "NonNumericRejection", "Validation"),
        "test_06_hex_address_format_validation": ("GUI_VAL_003", "HexAddressFormatValidation", "Validation"),
        "test_07_sector_parameter_validation": ("GUI_VAL_004", "SectorParameterValidation", "Validation"),
        "test_08_group_contiguous_sectors": ("GUI_GRP_001", "GroupContiguousSectors", "Grouping"),
        "test_09_split_on_different_size": ("GUI_GRP_002", "SplitOnDifferentSize", "Grouping"),
        "test_10_full_stm32f401_grouping": ("GUI_GRP_003", "FullSTM32F401Grouping", "Grouping"),
        "test_11_save_epc_structure": ("GUI_EPC_001", "SaveEpcStructure", "EPC File"),
        "test_12_load_epc_data_integrity": ("GUI_EPC_002", "LoadEpcDataIntegrity", "EPC File"),
        "test_13_corrupted_epc_handling": ("GUI_EPC_003", "CorruptedEpcHandling", "EPC File"),
        "test_14_generate_mem_cfg_h": ("GUI_GEN_001", "GenerateMemCfgH", "Code Gen"),
        "test_15_generate_mem_cfg_c": ("GUI_GEN_002", "GenerateMemCfgC", "Code Gen"),
        "test_16_generate_flash_ip_cfg": ("GUI_GEN_003", "GenerateFlashIpCfg", "Code Gen"),
        "test_17_compile_generated_code_std_on": ("GUI_E2E_001", "CompileGeneratedCodeStdOn", "End-to-End"),
        "test_18_compile_generated_code_std_off": ("GUI_E2E_002", "CompileGeneratedCodeStdOff", "End-to-End"),
    }
    
    print("\n" + "=" * 82, flush=True)
    print("        AUTOSAR MEM CONFIGURATOR & GENERATOR - AUTOMATED TEST REPORT", flush=True)
    print("=" * 82, flush=True)
    print(f" {'ID':<13} | {'Testcase Name':<30} | {'Group':<12} | {'Result':<8} | {'Time':<7}", flush=True)
    print("-" * 82, flush=True)
    
    # Initialize test class once for all tests
    TestMemConfiguratorAndGenerator.setUpClass()
    test_instance = TestMemConfiguratorAndGenerator()
    
    passed_count = 0
    failed_count = 0
    total_start = time.time()
    
    for method_name in sorted(tc_meta.keys()):
        tc_id, tc_name, tc_group = tc_meta[method_name]
        test_method = getattr(test_instance, method_name)
        
        t_start = time.time()
        err_msg = None
        try:
            test_method()
            status = "[ PASS ]"
            passed_count += 1
        except unittest.SkipTest as st:
            status = "[ SKIP ]"
            err_msg = str(st)
        except Exception as e:
            status = "[ FAIL ]"
            failed_count += 1
            err_msg = str(e)
            
        t_duration = time.time() - t_start
        print(f" {tc_id:<13} | {tc_name:<30} | {tc_group:<12} | {status:<8} | {t_duration:6.3f}s", flush=True)
        if err_msg:
            print(f"    --> INFO: {err_msg}", flush=True)
            
    total_duration = time.time() - total_start
    total_count = passed_count + failed_count
    pass_rate = (passed_count / total_count * 100) if total_count > 0 else 0
    
    print("=" * 82, flush=True)
    print(f" TOTAL: {total_count}  |  PASSED: {passed_count} ({pass_rate:.1f}%)  |  FAILED: {failed_count}  |  DURATION: {total_duration:.2f}s", flush=True)
    print("=" * 82 + "\n", flush=True)
    
    return 0 if failed_count == 0 else 1


if __name__ == "__main__":
    sys.exit(run_clean_test_suite())
