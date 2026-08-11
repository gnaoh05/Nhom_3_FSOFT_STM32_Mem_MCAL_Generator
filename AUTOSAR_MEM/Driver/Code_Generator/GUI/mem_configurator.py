#!/usr/bin/env python3
"""Single-file AUTOSAR Mem/Flash configuration GUI and code generator.

The application deliberately uses only the Python standard library so that it
can run on a normal Python installation without a package installation step.
It provides:

* a Tkinter desktop GUI;
* JSON project save/load and legacy EPD/EPC import;
* canonical AUTOSAR ECUC EPD-driven metadata and validation;
* AUTOSAR ECUC-style EPC configuration-value export;
* external-template Mem_Cfg.h/.c and Flash_IP_Cfg.h/.c generation;
* a headless self-test for CI and quick installation checks.

The EPD/EPC serializers target the AUTOSAR R4 namespace and the parameter model
used by this repository. Import into a particular commercial configurator still
needs to be checked against that tool's AUTOSAR release and vendor XSD/plugin.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import tempfile
import uuid
import xml.etree.ElementTree as ET
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Iterable, Sequence

try:
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk
except ImportError:  # Allows the headless CLI/self-test to remain usable.
    tk = None
    filedialog = messagebox = ttk = None


APP_NAME = "AUTOSAR Mem Configurator"
APP_VERSION = "2.0.0"
PROJECT_FORMAT = "autosar-mem-config/v1"
AUTOSAR_NS = "http://autosar.org/schema/r4.0"
XSI_NS = "http://www.w3.org/2001/XMLSchema-instance"
DEFINITION_ROOT = "/AUTOSAR_ECUC/Mem"
SCRIPT_DIR = Path(__file__).resolve().parent
CODE_GENERATOR_DIR = SCRIPT_DIR.parent
DEFAULT_EPD_PATH = SCRIPT_DIR / "Mem.epd"
DEFAULT_EPC_PATH = SCRIPT_DIR / "examples" / "STM32F401.epc"
DEFAULT_TEMPLATE_ROOT = CODE_GENERATOR_DIR / "templates"
DEFAULT_OUTPUT_ROOT = CODE_GENERATOR_DIR.parent.parent / "Test" / "Generated_File"
SUPPORTED_INVOCATIONS = (
    "DIRECT_STATIC",
    "INDIRECT_STATIC",
    "INDIRECT_DYNAMIC",
)

ET.register_namespace("", AUTOSAR_NS)
ET.register_namespace("xsi", XSI_NS)


class ConfigInputError(ValueError):
    """Raised when a GUI or project value cannot be converted."""


@dataclass
class Sector:
    start_address: int
    size: int

    @property
    def end_address(self) -> int:
        return self.start_address + self.size


@dataclass
class MemConfig:
    module_name: str = "Mem"
    autosar_release: str = "R25-11"
    dev_error_detect: bool = True
    version_info_api: bool = True
    mem_index: int = 0
    invocation: str = "DIRECT_STATIC"
    main_function_period: float = 0.005
    instance_name: str = "MemInstance_0"
    flash_base_address: int = 0x08000000
    flash_total_size: int = 0x00080000
    erased_value: int = 0xFF
    sectors: list[Sector] = field(default_factory=list)

    @classmethod
    def default(cls) -> "MemConfig":
        return cls(
            sectors=[
                Sector(0x08000000, 0x00004000),
                Sector(0x08004000, 0x00004000),
                Sector(0x08008000, 0x00004000),
                Sector(0x0800C000, 0x00004000),
                Sector(0x08010000, 0x00010000),
                Sector(0x08020000, 0x00020000),
                Sector(0x08040000, 0x00020000),
                Sector(0x08060000, 0x00020000),
            ]
        )

    def to_project_dict(self) -> dict:
        data = asdict(self)
        return {
            "format": PROJECT_FORMAT,
            "generator": f"{APP_NAME} {APP_VERSION}",
            "configuration": data,
        }

    @classmethod
    def from_project_dict(cls, project: dict) -> "MemConfig":
        if project.get("format") != PROJECT_FORMAT:
            raise ConfigInputError(
                f"Unsupported project format: {project.get('format')!r}"
            )
        data = dict(project.get("configuration") or {})
        raw_sectors = data.pop("sectors", [])
        try:
            sectors = [
                Sector(
                    parse_int(item["start_address"]),
                    parse_int(item["size"]),
                )
                for item in raw_sectors
            ]
            numeric_fields = (
                "mem_index",
                "flash_base_address",
                "flash_total_size",
                "erased_value",
            )
            for name in numeric_fields:
                if name in data:
                    data[name] = parse_int(data[name])
            if "main_function_period" in data:
                data["main_function_period"] = float(data["main_function_period"])
            config = cls(**data)
            config.sectors = sectors
            return config
        except (KeyError, TypeError, ValueError) as exc:
            raise ConfigInputError(f"Invalid project data: {exc}") from exc


@dataclass(frozen=True)
class ParameterDefinition:
    """One ECUC parameter definition loaded from the canonical EPD."""

    container: str
    name: str
    kind: str
    default: str
    minimum: str = ""
    maximum: str = ""
    literals: tuple[str, ...] = ()
    description: str = ""


@dataclass(frozen=True)
class ModuleDefinition:
    """EPD metadata used by the GUI, validator and code-generation pipeline."""

    short_name: str
    parameters: dict[str, ParameterDefinition]
    source_path: Path


def parse_int(value: object) -> int:
    """Parse decimal or 0x-prefixed integers, accepting common C suffixes."""
    if isinstance(value, bool):
        raise ValueError("boolean is not an integer value")
    if isinstance(value, int):
        return value
    text = str(value).strip().replace("_", "")
    text = re.sub(r"(?i)(UL|LU|U|L)$", "", text)
    if not text:
        raise ValueError("empty integer")
    return int(text, 0)


def format_hex(value: int, width: int = 8) -> str:
    return f"0x{value:0{width}X}"


def c_hex(value: int, width: int = 8) -> str:
    return f"{format_hex(value, width)}UL"


def bool_text(value: bool) -> str:
    return "true" if value else "false"


def c_bool(value: bool) -> str:
    return "STD_ON" if value else "STD_OFF"


def c_identifier(value: str) -> str:
    identifier = re.sub(r"\W", "_", value, flags=re.UNICODE)
    if not identifier:
        return "MemInstance_0"
    if identifier[0].isdigit():
        identifier = "_" + identifier
    return identifier


def validate_config(config: MemConfig) -> list[str]:
    errors: list[str] = []

    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", config.module_name):
        errors.append("Module name must be a valid AUTOSAR/C short name.")
    elif config.module_name != "Mem":
        errors.append("This MVP generates the Mem module, so Module name must be Mem.")
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", config.instance_name):
        errors.append("Instance name must contain only letters, digits and underscores.")
    if not config.autosar_release.strip():
        errors.append("AUTOSAR release must not be empty.")
    if not 0 <= config.mem_index <= 255:
        errors.append("MemIndex must be in the range 0..255.")
    if config.invocation not in SUPPORTED_INVOCATIONS:
        errors.append(f"Unknown MemInvocation value: {config.invocation}.")
    elif config.invocation != "DIRECT_STATIC":
        errors.append(
            "This reference driver implements only DIRECT_STATIC invocation."
        )
    if config.main_function_period <= 0:
        errors.append("MemMainFunctionPeriod must be greater than zero.")
    if not 0 <= config.flash_base_address <= 0xFFFFFFFF:
        errors.append("Flash base address must fit in uint32.")
    if not 0 < config.flash_total_size <= 0xFFFFFFFF:
        errors.append("Flash total size must be in the range 1..0xFFFFFFFF.")
    if config.flash_base_address + config.flash_total_size > 0x100000000:
        errors.append("Flash address range exceeds uint32 address space.")
    if not 0 <= config.erased_value <= 0xFF:
        errors.append("Erased value must fit in uint8 (0..0xFF).")
    if not config.sectors:
        errors.append("At least one Flash sector is required.")
        return errors
    if len(config.sectors) > 255:
        errors.append("Sector count must fit in uint8 (maximum 255).")

    expected_address = config.flash_base_address
    total_size = 0
    flash_end = config.flash_base_address + config.flash_total_size
    for index, sector in enumerate(config.sectors):
        label = f"Sector {index}"
        if not 0 <= sector.start_address <= 0xFFFFFFFF:
            errors.append(f"{label}: start address must fit in uint32.")
        if not 0 < sector.size <= 0xFFFFFFFF:
            errors.append(f"{label}: size must be in the range 1..0xFFFFFFFF.")
            continue
        if sector.start_address != expected_address:
            if sector.start_address < expected_address:
                errors.append(
                    f"{label}: overlaps the preceding sector "
                    f"(expected start {format_hex(expected_address)})."
                )
            else:
                errors.append(
                    f"{label}: leaves a gap before the sector "
                    f"(expected start {format_hex(expected_address)})."
                )
        if sector.end_address > flash_end:
            errors.append(f"{label}: ends outside the configured Flash range.")
        expected_address = sector.end_address
        total_size += sector.size

    if total_size != config.flash_total_size:
        errors.append(
            "Sum of sector sizes "
            f"({format_hex(total_size)}) does not equal Flash total size "
            f"({format_hex(config.flash_total_size)})."
        )
    if expected_address != flash_end:
        errors.append(
            "The sector table does not end at the configured Flash end address "
            f"({format_hex(flash_end)})."
        )
    return errors


def require_valid(
    config: MemConfig, definition: ModuleDefinition | None = None
) -> None:
    errors = validate_config(config)
    if definition is not None:
        errors.extend(validate_config_against_definition(config, definition))
    if errors:
        raise ConfigInputError("\n".join(errors))


def qname(tag: str) -> str:
    return f"{{{AUTOSAR_NS}}}{tag}"


def xml_child(
    parent: ET.Element,
    tag: str,
    text: object | None = None,
    attrib: dict[str, str] | None = None,
) -> ET.Element:
    element = ET.SubElement(parent, qname(tag), attrib or {})
    if text is not None:
        element.text = str(text)
    return element


def xml_desc(parent: ET.Element, text: str) -> None:
    desc = xml_child(parent, "DESC")
    xml_child(desc, "L-2", text, {"L": "EN"})


def stable_uuid(path: str) -> str:
    return str(uuid.uuid5(uuid.NAMESPACE_URL, f"autosar-mem-configurator:{path}")).upper()


def add_multiplicity(
    parent: ET.Element, lower: int, upper: int | None
) -> None:
    xml_child(parent, "LOWER-MULTIPLICITY", lower)
    if upper is None:
        xml_child(parent, "UPPER-MULTIPLICITY-INFINITE", "true")
    else:
        xml_child(parent, "UPPER-MULTIPLICITY", upper)


def add_boolean_def(
    parameters: ET.Element, name: str, default: bool, description: str
) -> None:
    node = xml_child(
        parameters,
        "ECUC-BOOLEAN-PARAM-DEF",
        attrib={"UUID": stable_uuid(f"definition/{name}")},
    )
    xml_child(node, "SHORT-NAME", name)
    xml_desc(node, description)
    add_multiplicity(node, 1, 1)
    xml_child(node, "DEFAULT-VALUE", bool_text(default))
    xml_child(node, "ORIGIN", "AUTOSAR_ECUC")


def add_integer_def(
    parameters: ET.Element,
    name: str,
    default: int,
    minimum: int,
    maximum: int,
    description: str,
) -> None:
    node = xml_child(
        parameters,
        "ECUC-INTEGER-PARAM-DEF",
        attrib={"UUID": stable_uuid(f"definition/{name}")},
    )
    xml_child(node, "SHORT-NAME", name)
    xml_desc(node, description)
    add_multiplicity(node, 1, 1)
    xml_child(node, "DEFAULT-VALUE", default)
    xml_child(node, "MAX", maximum)
    xml_child(node, "MIN", minimum)
    xml_child(node, "ORIGIN", "AUTOSAR_ECUC")


def add_float_def(
    parameters: ET.Element,
    name: str,
    default: float,
    minimum: float,
    description: str,
) -> None:
    node = xml_child(
        parameters,
        "ECUC-FLOAT-PARAM-DEF",
        attrib={"UUID": stable_uuid(f"definition/{name}")},
    )
    xml_child(node, "SHORT-NAME", name)
    xml_desc(node, description)
    add_multiplicity(node, 1, 1)
    xml_child(node, "DEFAULT-VALUE", f"{default:.9g}")
    xml_child(node, "MIN", f"{minimum:.9g}")
    xml_child(node, "ORIGIN", "AUTOSAR_ECUC")


def add_enum_def(
    parameters: ET.Element,
    name: str,
    default: str,
    literals: Sequence[str],
    description: str,
) -> None:
    node = xml_child(
        parameters,
        "ECUC-ENUMERATION-PARAM-DEF",
        attrib={"UUID": stable_uuid(f"definition/{name}")},
    )
    xml_child(node, "SHORT-NAME", name)
    xml_desc(node, description)
    add_multiplicity(node, 1, 1)
    xml_child(node, "DEFAULT-VALUE", default)
    literals_node = xml_child(node, "LITERALS")
    for literal in literals:
        literal_node = xml_child(
            literals_node,
            "ECUC-ENUMERATION-LITERAL-DEF",
            attrib={"UUID": stable_uuid(f"definition/{name}/{literal}")},
        )
        xml_child(literal_node, "SHORT-NAME", literal)
    xml_child(node, "ORIGIN", "AUTOSAR_ECUC")


def create_epd_tree() -> ET.ElementTree:
    """Create the stable ECUC module definition; project values belong in EPC."""
    config = MemConfig.default()
    require_valid(config)
    root = ET.Element(qname("AUTOSAR"))
    root.append(
        ET.Comment(
            f" Generated by {APP_NAME} {APP_VERSION}; "
            f"target project release {config.autosar_release}. "
        )
    )
    packages = xml_child(root, "AR-PACKAGES")
    package = xml_child(packages, "AR-PACKAGE")
    xml_child(package, "SHORT-NAME", "AUTOSAR_ECUC")
    elements = xml_child(package, "ELEMENTS")
    module = xml_child(
        elements,
        "ECUC-MODULE-DEF",
        attrib={"UUID": stable_uuid("definition/Mem")},
    )
    xml_child(module, "SHORT-NAME", config.module_name)
    xml_desc(
        module,
        "ECUC parameter definitions for the AUTOSAR Mem reference driver "
        "and STM32F401 internal Flash integration.",
    )
    xml_child(module, "API-SERVICE-PREFIX", "Mem")
    variants = xml_child(module, "SUPPORTED-CONFIG-VARIANTS")
    xml_child(variants, "SUPPORTED-CONFIG-VARIANT", "VARIANT-PRE-COMPILE")

    containers = xml_child(module, "CONTAINERS")
    general = xml_child(
        containers,
        "ECUC-PARAM-CONF-CONTAINER-DEF",
        attrib={"UUID": stable_uuid("definition/MemGeneral")},
    )
    xml_child(general, "SHORT-NAME", "MemGeneral")
    xml_desc(general, "General pre-compile configuration of the Mem module.")
    add_multiplicity(general, 1, 1)
    general_params = xml_child(general, "PARAMETERS")
    add_boolean_def(
        general_params,
        "MemDevErrorDetect",
        config.dev_error_detect,
        "Enable development error detection.",
    )
    add_boolean_def(
        general_params,
        "MemVersionInfoApi",
        config.version_info_api,
        "Enable the Mem_GetVersionInfo API.",
    )
    add_integer_def(
        general_params,
        "MemIndex",
        config.mem_index,
        0,
        255,
        "Instance identifier passed to DET.",
    )
    add_enum_def(
        general_params,
        "MemInvocation",
        config.invocation,
        SUPPORTED_INVOCATIONS,
        "Invocation method. The repository implementation supports DIRECT_STATIC.",
    )
    add_float_def(
        general_params,
        "MemMainFunctionPeriod",
        config.main_function_period,
        0.000001,
        "Period of Mem_MainFunction in seconds.",
    )

    instance = xml_child(
        containers,
        "ECUC-PARAM-CONF-CONTAINER-DEF",
        attrib={"UUID": stable_uuid("definition/MemInstance")},
    )
    xml_child(instance, "SHORT-NAME", "MemInstance")
    xml_desc(instance, "One Mem driver instance backed by internal Flash.")
    add_multiplicity(instance, 1, 1)
    instance_params = xml_child(instance, "PARAMETERS")
    add_integer_def(
        instance_params,
        "MemInstanceId",
        0,
        0,
        0,
        "Symbolic identifier of the single supported Mem instance.",
    )
    add_integer_def(
        instance_params,
        "MemStartAddress",
        config.flash_base_address,
        0,
        0xFFFFFFFF,
        "Start address of the memory instance.",
    )
    add_integer_def(
        instance_params,
        "MemSize",
        config.flash_total_size,
        1,
        0xFFFFFFFF,
        "Total size of the memory instance in bytes.",
    )
    subcontainers = xml_child(instance, "SUB-CONTAINERS")
    sector = xml_child(
        subcontainers,
        "ECUC-PARAM-CONF-CONTAINER-DEF",
        attrib={"UUID": stable_uuid("definition/MemInstance/MemSectorBatch")},
    )
    xml_child(sector, "SHORT-NAME", "MemSectorBatch")
    xml_desc(sector, "Physical Flash sector geometry.")
    add_multiplicity(sector, 1, None)
    sector_params = xml_child(sector, "PARAMETERS")
    add_integer_def(
        sector_params,
        "MemSectorStartAddress",
        config.flash_base_address,
        0,
        0xFFFFFFFF,
        "Absolute start address of the Flash sector.",
    )
    add_integer_def(
        sector_params,
        "MemSectorSize",
        config.sectors[0].size,
        1,
        0xFFFFFFFF,
        "Size of the Flash sector in bytes.",
    )

    published = xml_child(
        containers,
        "ECUC-PARAM-CONF-CONTAINER-DEF",
        attrib={"UUID": stable_uuid("definition/MemPublishedInformation")},
    )
    xml_child(published, "SHORT-NAME", "MemPublishedInformation")
    xml_desc(published, "Published information of the configured memory.")
    add_multiplicity(published, 1, 1)
    published_params = xml_child(published, "PARAMETERS")
    add_integer_def(
        published_params,
        "MemErasedValue",
        config.erased_value,
        0,
        255,
        "Value read from one erased memory cell.",
    )
    return ET.ElementTree(root)


def add_numerical_value(
    values: ET.Element,
    name: str,
    definition_container: str,
    definition_type: str,
    value: object,
) -> None:
    node = xml_child(values, "ECUC-NUMERICAL-PARAM-VALUE")
    xml_child(
        node,
        "DEFINITION-REF",
        f"{DEFINITION_ROOT}/{definition_container}/{name}",
        {"DEST": definition_type},
    )
    xml_child(node, "VALUE", value)


def add_textual_value(
    values: ET.Element,
    name: str,
    definition_container: str,
    definition_type: str,
    value: str,
) -> None:
    node = xml_child(values, "ECUC-TEXTUAL-PARAM-VALUE")
    xml_child(
        node,
        "DEFINITION-REF",
        f"{DEFINITION_ROOT}/{definition_container}/{name}",
        {"DEST": definition_type},
    )
    xml_child(node, "VALUE", value)


def add_container_value(
    parent: ET.Element,
    short_name: str,
    definition_path: str,
    definition_type: str = "ECUC-PARAM-CONF-CONTAINER-DEF",
) -> ET.Element:
    container = xml_child(parent, "ECUC-CONTAINER-VALUE")
    xml_child(container, "SHORT-NAME", short_name)
    xml_child(
        container,
        "DEFINITION-REF",
        definition_path,
        {"DEST": definition_type},
    )
    return container


def create_epc_tree(config: MemConfig) -> ET.ElementTree:
    """Create an ECUC configuration-value document with an .epc extension."""
    require_valid(config)
    root = ET.Element(qname("AUTOSAR"))
    root.append(
        ET.Comment(
            f" Generated by {APP_NAME} {APP_VERSION}; "
            f"target project release {config.autosar_release}. "
        )
    )
    packages = xml_child(root, "AR-PACKAGES")
    package = xml_child(packages, "AR-PACKAGE")
    xml_child(package, "SHORT-NAME", "Mem_Configuration")
    elements = xml_child(package, "ELEMENTS")
    module = xml_child(
        elements,
        "ECUC-MODULE-CONFIGURATION-VALUES",
        attrib={"UUID": stable_uuid("configuration/Mem")},
    )
    xml_child(module, "SHORT-NAME", config.module_name)
    xml_child(
        module,
        "DEFINITION-REF",
        DEFINITION_ROOT,
        {"DEST": "ECUC-MODULE-DEF"},
    )
    xml_child(module, "IMPLEMENTATION-CONFIG-VARIANT", "VARIANT-PRE-COMPILE")
    containers = xml_child(module, "CONTAINERS")

    general = add_container_value(
        containers, "MemGeneral", f"{DEFINITION_ROOT}/MemGeneral"
    )
    params = xml_child(general, "PARAMETER-VALUES")
    add_numerical_value(
        params,
        "MemDevErrorDetect",
        "MemGeneral",
        "ECUC-BOOLEAN-PARAM-DEF",
        bool_text(config.dev_error_detect),
    )
    add_numerical_value(
        params,
        "MemVersionInfoApi",
        "MemGeneral",
        "ECUC-BOOLEAN-PARAM-DEF",
        bool_text(config.version_info_api),
    )
    add_numerical_value(
        params,
        "MemIndex",
        "MemGeneral",
        "ECUC-INTEGER-PARAM-DEF",
        config.mem_index,
    )
    add_textual_value(
        params,
        "MemInvocation",
        "MemGeneral",
        "ECUC-ENUMERATION-PARAM-DEF",
        config.invocation,
    )
    add_numerical_value(
        params,
        "MemMainFunctionPeriod",
        "MemGeneral",
        "ECUC-FLOAT-PARAM-DEF",
        f"{config.main_function_period:.9g}",
    )

    instance = add_container_value(
        containers,
        config.instance_name,
        f"{DEFINITION_ROOT}/MemInstance",
    )
    params = xml_child(instance, "PARAMETER-VALUES")
    add_numerical_value(
        params,
        "MemInstanceId",
        "MemInstance",
        "ECUC-INTEGER-PARAM-DEF",
        0,
    )
    add_numerical_value(
        params,
        "MemStartAddress",
        "MemInstance",
        "ECUC-INTEGER-PARAM-DEF",
        config.flash_base_address,
    )
    add_numerical_value(
        params,
        "MemSize",
        "MemInstance",
        "ECUC-INTEGER-PARAM-DEF",
        config.flash_total_size,
    )
    subcontainers = xml_child(instance, "SUB-CONTAINERS")
    for index, item in enumerate(config.sectors):
        sector = add_container_value(
            subcontainers,
            f"MemSectorBatch_{index}",
            f"{DEFINITION_ROOT}/MemInstance/MemSectorBatch",
        )
        sector_values = xml_child(sector, "PARAMETER-VALUES")
        add_numerical_value(
            sector_values,
            "MemSectorStartAddress",
            "MemInstance/MemSectorBatch",
            "ECUC-INTEGER-PARAM-DEF",
            item.start_address,
        )
        add_numerical_value(
            sector_values,
            "MemSectorSize",
            "MemInstance/MemSectorBatch",
            "ECUC-INTEGER-PARAM-DEF",
            item.size,
        )

    published = add_container_value(
        containers,
        "MemPublishedInformation",
        f"{DEFINITION_ROOT}/MemPublishedInformation",
    )
    params = xml_child(published, "PARAMETER-VALUES")
    add_numerical_value(
        params,
        "MemErasedValue",
        "MemPublishedInformation",
        "ECUC-INTEGER-PARAM-DEF",
        config.erased_value,
    )
    return ET.ElementTree(root)


def write_xml(tree: ET.ElementTree, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    ET.indent(tree, space="  ")
    tree.write(path, encoding="utf-8", xml_declaration=True)


def save_project(config: MemConfig, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(config.to_project_dict(), indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def load_project(path: Path) -> MemConfig:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ConfigInputError(f"Cannot read project: {exc}") from exc
    return MemConfig.from_project_dict(data)


def parse_bool(value: object) -> bool:
    """Parse the boolean spellings used by JSON, EPD/EPC and AUTOSAR C macros."""
    if isinstance(value, bool):
        return value
    normalized = str(value).strip().upper()
    if normalized in {"TRUE", "1", "STD_ON", "ON", "YES"}:
        return True
    if normalized in {"FALSE", "0", "STD_OFF", "OFF", "NO"}:
        return False
    raise ValueError(f"invalid boolean value: {value!r}")


def xml_local_name(tag: str) -> str:
    """Return an XML tag without its optional namespace."""
    return tag.rsplit("}", 1)[-1]


def xml_direct_child(parent: ET.Element, name: str) -> ET.Element | None:
    for child in parent:
        if xml_local_name(child.tag) == name:
            return child
    return None


def xml_direct_text(parent: ET.Element, name: str, default: str = "") -> str:
    child = xml_direct_child(parent, name)
    if child is None or child.text is None:
        return default
    return child.text.strip()


def xml_description(element: ET.Element) -> str:
    for child in element.iter():
        if xml_local_name(child.tag) == "L-2" and child.text:
            return child.text.strip()
    return ""


def load_module_definition(path: Path = DEFAULT_EPD_PATH) -> ModuleDefinition:
    """Load ECUC parameter metadata from the canonical AUTOSAR EPD."""
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError) as exc:
        raise ConfigInputError(f"Cannot read EPD definition {path}: {exc}") from exc

    module = next(
        (
            element
            for element in root.iter()
            if xml_local_name(element.tag) == "ECUC-MODULE-DEF"
        ),
        None,
    )
    if module is None:
        raise ConfigInputError(f"No ECUC-MODULE-DEF found in EPD: {path}")

    parameters: dict[str, ParameterDefinition] = {}
    for container in module.iter():
        if xml_local_name(container.tag) != "ECUC-PARAM-CONF-CONTAINER-DEF":
            continue
        container_name = xml_direct_text(container, "SHORT-NAME")
        parameter_nodes = xml_direct_child(container, "PARAMETERS")
        if parameter_nodes is None:
            continue
        for parameter in parameter_nodes:
            tag = xml_local_name(parameter.tag)
            if not tag.endswith("-PARAM-DEF"):
                continue
            name = xml_direct_text(parameter, "SHORT-NAME")
            if not name:
                continue
            if name in parameters:
                raise ConfigInputError(f"Duplicate EPD parameter definition: {name}")
            if "BOOLEAN" in tag:
                kind = "BOOLEAN"
            elif "INTEGER" in tag:
                kind = "INTEGER"
            elif "FLOAT" in tag:
                kind = "FLOAT"
            elif "ENUMERATION" in tag:
                kind = "ENUMERATION"
            else:
                kind = "STRING"
            literals = tuple(
                xml_direct_text(literal, "SHORT-NAME")
                for literal in parameter.iter()
                if xml_local_name(literal.tag) == "ECUC-ENUMERATION-LITERAL-DEF"
                and xml_direct_text(literal, "SHORT-NAME")
            )
            parameters[name] = ParameterDefinition(
                container=container_name,
                name=name,
                kind=kind,
                default=xml_direct_text(parameter, "DEFAULT-VALUE"),
                minimum=xml_direct_text(parameter, "MIN"),
                maximum=xml_direct_text(parameter, "MAX"),
                literals=literals,
                description=xml_description(parameter),
            )

    definition = ModuleDefinition(
        short_name=xml_direct_text(module, "SHORT-NAME"),
        parameters=parameters,
        source_path=path,
    )
    errors = validate_module_definition(definition)
    if errors:
        raise ConfigInputError("Invalid EPD definition:\n- " + "\n- ".join(errors))
    return definition


def validate_module_definition(definition: ModuleDefinition) -> list[str]:
    expected_kinds = {
        "MemDevErrorDetect": "BOOLEAN",
        "MemVersionInfoApi": "BOOLEAN",
        "MemIndex": "INTEGER",
        "MemInvocation": "ENUMERATION",
        "MemMainFunctionPeriod": "FLOAT",
        "MemInstanceId": "INTEGER",
        "MemStartAddress": "INTEGER",
        "MemSize": "INTEGER",
        "MemSectorStartAddress": "INTEGER",
        "MemSectorSize": "INTEGER",
        "MemErasedValue": "INTEGER",
    }
    errors: list[str] = []
    if definition.short_name != "Mem":
        errors.append(f"EPD module short name must be Mem, got {definition.short_name!r}.")
    for name, expected_kind in expected_kinds.items():
        parameter = definition.parameters.get(name)
        if parameter is None:
            errors.append(f"Missing EPD parameter definition: {name}.")
        elif parameter.kind != expected_kind:
            errors.append(
                f"EPD parameter {name} must be {expected_kind}, got {parameter.kind}."
            )
    invocation = definition.parameters.get("MemInvocation")
    if invocation is not None:
        missing = [value for value in SUPPORTED_INVOCATIONS if value not in invocation.literals]
        if missing:
            errors.append("MemInvocation EPD literals are missing: " + ", ".join(missing))
    return errors


def validate_definition_value(
    parameter: ParameterDefinition, value: object, label: str
) -> list[str]:
    errors: list[str] = []
    try:
        if parameter.kind == "BOOLEAN":
            parse_bool(value)
        elif parameter.kind == "ENUMERATION":
            if str(value) not in parameter.literals:
                errors.append(
                    f"{label}: {value!r} is not an EPD literal for {parameter.name}."
                )
        elif parameter.kind == "INTEGER":
            numeric = parse_int(value)
            if parameter.minimum and numeric < parse_int(parameter.minimum):
                errors.append(f"{label}: value is below EPD minimum {parameter.minimum}.")
            if parameter.maximum and numeric > parse_int(parameter.maximum):
                errors.append(f"{label}: value exceeds EPD maximum {parameter.maximum}.")
        elif parameter.kind == "FLOAT":
            numeric_float = float(value)
            if parameter.minimum and numeric_float < float(parameter.minimum):
                errors.append(f"{label}: value is below EPD minimum {parameter.minimum}.")
            if parameter.maximum and numeric_float > float(parameter.maximum):
                errors.append(f"{label}: value exceeds EPD maximum {parameter.maximum}.")
    except (TypeError, ValueError) as exc:
        errors.append(f"{label}: cannot validate against EPD: {exc}.")
    return errors


def validate_config_against_definition(
    config: MemConfig, definition: ModuleDefinition
) -> list[str]:
    errors = validate_module_definition(definition)
    if errors:
        return errors
    values: dict[str, object] = {
        "MemDevErrorDetect": config.dev_error_detect,
        "MemVersionInfoApi": config.version_info_api,
        "MemIndex": config.mem_index,
        "MemInvocation": config.invocation,
        "MemMainFunctionPeriod": config.main_function_period,
        "MemInstanceId": 0,
        "MemStartAddress": config.flash_base_address,
        "MemSize": config.flash_total_size,
        "MemErasedValue": config.erased_value,
    }
    for name, value in values.items():
        errors.extend(validate_definition_value(definition.parameters[name], value, name))
    for index, sector in enumerate(config.sectors):
        errors.extend(
            validate_definition_value(
                definition.parameters["MemSectorStartAddress"],
                sector.start_address,
                f"Sector {index} start",
            )
        )
        errors.extend(
            validate_definition_value(
                definition.parameters["MemSectorSize"],
                sector.size,
                f"Sector {index} size",
            )
        )
    return errors


def legacy_parameter_map(container: ET.Element | None) -> dict[str, str]:
    """Read parameters from the custom XML format used by gui-code-generation."""
    if container is None:
        return {}
    result: dict[str, str] = {}
    for parameter in container.findall("Parameter"):
        name = parameter.get("name")
        if name:
            result[name] = parameter.get("value", parameter.get("default", ""))
    return result


def expand_legacy_sectors(items: Iterable[ET.Element]) -> list[Sector]:
    """Expand legacy MemSectorBatch rows into the physical sector table."""
    sectors: list[Sector] = []
    for item in items:
        try:
            start = parse_int(item.get("MemStartAddress", ""))
            size = parse_int(item.get("MemEraseSectorSize", ""))
            count = parse_int(item.get("MemNumberOfSectors", "1"))
        except ValueError as exc:
            raise ConfigInputError(f"Invalid legacy sector row: {exc}") from exc
        if count < 1:
            raise ConfigInputError("Legacy MemNumberOfSectors must be at least 1.")
        sectors.extend(Sector(start + offset * size, size) for offset in range(count))
    return sorted(sectors, key=lambda sector: sector.start_address)


def apply_imported_values(
    values: dict[str, dict[str, str]],
    sectors: list[Sector],
    instance_name: str = "",
) -> MemConfig:
    """Map the common legacy/AUTOSAR parameter names to the current driver model."""
    config = MemConfig.default()
    general = values.get("MemGeneral", {})
    instance = values.get("MemInstance", {})
    published = values.get("MemPublishedInformation", {})

    try:
        if "MemDevErrorDetect" in general:
            config.dev_error_detect = parse_bool(general["MemDevErrorDetect"])
        if "MemVersionInfoApi" in general:
            config.version_info_api = parse_bool(general["MemVersionInfoApi"])
        if "MemIndex" in general:
            config.mem_index = parse_int(general["MemIndex"])
        if general.get("MemInvocation"):
            config.invocation = general["MemInvocation"].strip()
        if general.get("MemMainFunctionPeriod"):
            config.main_function_period = float(general["MemMainFunctionPeriod"])

        erased_value = general.get("MemErasedValue", published.get("MemErasedValue"))
        if erased_value not in (None, ""):
            config.erased_value = parse_int(erased_value)

        if instance_name:
            config.instance_name = c_identifier(instance_name)
        elif instance.get("MemInstanceId") not in (None, ""):
            config.instance_name = f"MemInstance_{parse_int(instance['MemInstanceId'])}"

        if sectors:
            config.sectors = sectors
            config.flash_base_address = sectors[0].start_address
            config.flash_total_size = sectors[-1].end_address - sectors[0].start_address
        else:
            base = instance.get("MemStartAddress", instance.get("MemBaseAddress"))
            size = instance.get("MemSize", instance.get("MemTotalSize"))
            if base not in (None, ""):
                config.flash_base_address = parse_int(base)
            if size not in (None, ""):
                config.flash_total_size = parse_int(size)
    except (TypeError, ValueError) as exc:
        raise ConfigInputError(f"Invalid imported configuration value: {exc}") from exc
    return config


def load_legacy_xml(root: ET.Element) -> MemConfig:
    """Import the custom AUTOSAR_EPD/AUTOSAR_EPC dialect from the member branch."""
    module = root.find("Module")
    if module is None:
        raise ConfigInputError("Legacy EPD/EPC has no Module element.")

    values: dict[str, dict[str, str]] = {}
    sectors: list[Sector] = []
    for container in module.findall("Container"):
        name = container.get("name", "")
        if name:
            values[name] = legacy_parameter_map(container)
        for sector_list in container.findall("List"):
            if sector_list.get("name") == "MemSectorBatch":
                sectors.extend(expand_legacy_sectors(sector_list.findall("Item")))

    # The member tool calls the module MemDriver; the integrated C driver and
    # AUTOSAR definition use the required short name Mem, so it is normalized.
    return apply_imported_values(values, sorted(sectors, key=lambda item: item.start_address))


def autosar_parameter_values(container: ET.Element) -> dict[str, str]:
    values: dict[str, str] = {}
    parameter_values = xml_direct_child(container, "PARAMETER-VALUES")
    if parameter_values is None:
        return values
    for parameter in parameter_values:
        if xml_local_name(parameter.tag) not in {
            "ECUC-NUMERICAL-PARAM-VALUE",
            "ECUC-TEXTUAL-PARAM-VALUE",
        }:
            continue
        definition = xml_direct_text(parameter, "DEFINITION-REF")
        value = xml_direct_text(parameter, "VALUE")
        if definition:
            values[definition.rsplit("/", 1)[-1]] = value
    return values


def load_autosar_epc(root: ET.Element) -> MemConfig:
    """Import standard ECUC configuration values, including files exported here."""
    module = next(
        (
            element
            for element in root.iter()
            if xml_local_name(element.tag) == "ECUC-MODULE-CONFIGURATION-VALUES"
        ),
        None,
    )
    if module is None:
        if any(xml_local_name(element.tag) == "ECUC-MODULE-DEF" for element in root.iter()):
            raise ConfigInputError(
                "This is an AUTOSAR parameter-definition EPD, not a concrete EPC configuration."
            )
        raise ConfigInputError("No ECUC-MODULE-CONFIGURATION-VALUES found in AUTOSAR XML.")

    values: dict[str, dict[str, str]] = {}
    sectors: list[Sector] = []
    instance_name = ""
    for container in module.iter():
        if xml_local_name(container.tag) != "ECUC-CONTAINER-VALUE":
            continue
        definition = xml_direct_text(container, "DEFINITION-REF")
        container_name = definition.rsplit("/", 1)[-1]
        params = autosar_parameter_values(container)
        if container_name == "MemSectorBatch":
            if "MemSectorStartAddress" in params and "MemSectorSize" in params:
                try:
                    sectors.append(
                        Sector(
                            parse_int(params["MemSectorStartAddress"]),
                            parse_int(params["MemSectorSize"]),
                        )
                    )
                except ValueError as exc:
                    raise ConfigInputError(f"Invalid AUTOSAR sector value: {exc}") from exc
            continue
        if container_name:
            values[container_name] = params
        if container_name == "MemInstance":
            instance_name = xml_direct_text(container, "SHORT-NAME")

    return apply_imported_values(
        values,
        sorted(sectors, key=lambda item: item.start_address),
        instance_name,
    )


def load_xml_configuration(path: Path) -> MemConfig:
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError) as exc:
        raise ConfigInputError(f"Cannot read XML configuration: {exc}") from exc
    root_name = xml_local_name(root.tag)
    if root_name in {"AUTOSAR_EPD", "AUTOSAR_EPC"}:
        return load_legacy_xml(root)
    if root_name == "AUTOSAR":
        return load_autosar_epc(root)
    raise ConfigInputError(f"Unsupported XML root element: {root_name!r}")


def load_configuration(path: Path) -> MemConfig:
    """Load the native JSON project or either supported EPD/EPC XML format."""
    if path.suffix.lower() == ".json":
        return load_project(path)
    return load_xml_configuration(path)


TEMPLATE_TOKEN_RE = re.compile(r"\{\{([A-Z0-9_]+)\}\}")
TEMPLATE_FILE_NAMES = {
    "mem_h": Path("include") / "Mem_Cfg.h.template",
    "mem_c": Path("src") / "Mem_Cfg.c.template",
    "flash_h": Path("include") / "Flash_IP_Cfg.h.template",
    "flash_c": Path("src") / "Flash_IP_Cfg.c.template",
}


def sector_table_rows(config: MemConfig) -> str:
    rows: list[str] = []
    for index, sector in enumerate(config.sectors):
        size_label = (
            f"{sector.size // 1024} KB" if sector.size % 1024 == 0 else f"{sector.size} bytes"
        )
        rows.append(
            f"    {{ {c_hex(sector.start_address)}, {c_hex(sector.size)} }}"
            f"{',' if index < len(config.sectors) - 1 else ' '} "
            f"/* Sector {index}: {size_label} */"
        )
    return "\n".join(rows)


def build_template_context(config: MemConfig) -> dict[str, str]:
    instance_macro = c_identifier(config.instance_name)
    return {
        "APP_NAME": APP_NAME,
        "APP_VERSION": APP_VERSION,
        "AUTOSAR_RELEASE": config.autosar_release,
        "MEM_DEV_ERROR_DETECT": c_bool(config.dev_error_detect),
        "MEM_VERSION_INFO_API": c_bool(config.version_info_api),
        "MEM_INDEX": f"{config.mem_index}u",
        "MEM_INVOCATION": f"MEM_INVOCATION_{config.invocation}",
        "MEM_MAIN_FUNCTION_PERIOD": f"{config.main_function_period:.9g}f",
        "MEM_INSTANCE_SYMBOL": f"MemConf_MemInstance_{instance_macro}",
        "FLASH_BASE_ADDRESS": c_hex(config.flash_base_address),
        "FLASH_TOTAL_SIZE": c_hex(config.flash_total_size),
        "FLASH_SECTOR_COUNT": f"{len(config.sectors)}u",
        "ERASED_VALUE": f"0x{config.erased_value:02X}u",
        "SECTOR_TABLE_ROWS": sector_table_rows(config),
        "SECTOR_TOTAL_SIZE_SUM": wrap_c_sum(sector.size for sector in config.sectors),
    }


def render_code_template(
    relative_path: Path,
    context: dict[str, str],
    template_root: Path = DEFAULT_TEMPLATE_ROOT,
) -> str:
    path = template_root / relative_path
    try:
        source = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise ConfigInputError(f"Cannot read code template {path}: {exc}") from exc
    required = set(TEMPLATE_TOKEN_RE.findall(source))
    missing = sorted(required.difference(context))
    if missing:
        raise ConfigInputError(
            f"Template {path.name} uses unknown token(s): {', '.join(missing)}"
        )
    rendered = TEMPLATE_TOKEN_RE.sub(lambda match: context[match.group(1)], source)
    unresolved = TEMPLATE_TOKEN_RE.findall(rendered)
    if unresolved:
        raise ConfigInputError(
            f"Template {path.name} has unresolved token(s): {', '.join(sorted(set(unresolved)))}"
        )
    return rendered.replace("\r\n", "\n")


def generate_mem_cfg_h(
    config: MemConfig, template_root: Path = DEFAULT_TEMPLATE_ROOT
) -> str:
    return render_code_template(
        TEMPLATE_FILE_NAMES["mem_h"], build_template_context(config), template_root
    )


def generate_mem_cfg_c(
    config: MemConfig, template_root: Path = DEFAULT_TEMPLATE_ROOT
) -> str:
    return render_code_template(
        TEMPLATE_FILE_NAMES["mem_c"], build_template_context(config), template_root
    )


def generate_flash_cfg_h(
    config: MemConfig, template_root: Path = DEFAULT_TEMPLATE_ROOT
) -> str:
    return render_code_template(
        TEMPLATE_FILE_NAMES["flash_h"], build_template_context(config), template_root
    )


def wrap_c_sum(values: Iterable[int]) -> str:
    terms = [c_hex(value) for value in values]
    lines: list[str] = []
    first_line = True
    while terms:
        batch, terms = terms[:3], terms[3:]
        suffix = " + \\" if terms else ""
        prefix = "        (" if first_line else "         "
        lines.append(prefix + " + ".join(batch) + suffix)
        first_line = False
    if not lines:
        return "        (0UL)"
    lines[-1] += ")"
    return "\n".join(lines)


def generate_flash_cfg_c(
    config: MemConfig, template_root: Path = DEFAULT_TEMPLATE_ROOT
) -> str:
    return render_code_template(
        TEMPLATE_FILE_NAMES["flash_c"], build_template_context(config), template_root
    )


def generated_c_paths(output_root: Path) -> dict[Path, str]:
    return {
        output_root / "include" / "Mem_Cfg.h": "mem_h",
        output_root / "src" / "Mem_Cfg.c": "mem_c",
        output_root / "include" / "Flash_IP_Cfg.h": "flash_h",
        output_root / "src" / "Flash_IP_Cfg.c": "flash_c",
    }


def write_c_files(
    config: MemConfig,
    output_root: Path,
    definition_path: Path = DEFAULT_EPD_PATH,
    template_root: Path = DEFAULT_TEMPLATE_ROOT,
) -> list[Path]:
    definition = load_module_definition(definition_path)
    require_valid(config, definition)
    content = {
        "mem_h": generate_mem_cfg_h(config, template_root),
        "mem_c": generate_mem_cfg_c(config, template_root),
        "flash_h": generate_flash_cfg_h(config, template_root),
        "flash_c": generate_flash_cfg_c(config, template_root),
    }
    paths: list[Path] = []
    for path, key in generated_c_paths(output_root).items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content[key], encoding="utf-8", newline="\n")
        paths.append(path)
    return paths


if tk is not None:

    class MemConfiguratorApp(tk.Tk):
        def __init__(self) -> None:
            super().__init__()
            self.title(f"{APP_NAME} {APP_VERSION}")
            self.geometry("1040x720")
            self.minsize(900, 620)
            self.definition = load_module_definition(DEFAULT_EPD_PATH)
            self.project_path: Path | None = None
            self.sectors: list[Sector] = []
            self._create_variables()
            self._create_style()
            self._create_menu()
            self._create_widgets()
            initial_config = load_configuration(DEFAULT_EPC_PATH)
            require_valid(initial_config, self.definition)
            self.load_config_into_ui(initial_config)

        def _create_variables(self) -> None:
            self.module_name_var = tk.StringVar()
            self.release_var = tk.StringVar()
            self.det_var = tk.BooleanVar()
            self.version_api_var = tk.BooleanVar()
            self.mem_index_var = tk.StringVar()
            self.invocation_var = tk.StringVar()
            self.period_var = tk.StringVar()
            self.instance_name_var = tk.StringVar()
            self.base_var = tk.StringVar()
            self.total_size_var = tk.StringVar()
            self.erased_var = tk.StringVar()
            self.sector_start_var = tk.StringVar()
            self.sector_size_var = tk.StringVar()
            self.status_var = tk.StringVar(value="Ready")

        def _create_style(self) -> None:
            style = ttk.Style(self)
            if "vista" in style.theme_names():
                style.theme_use("vista")
            style.configure("Title.TLabel", font=("Segoe UI", 16, "bold"))
            style.configure("Hint.TLabel", foreground="#555555")

        def _create_menu(self) -> None:
            menu = tk.Menu(self)
            file_menu = tk.Menu(menu, tearoff=False)
            file_menu.add_command(label="New", command=self.new_project, accelerator="Ctrl+N")
            file_menu.add_command(label="Open / import...", command=self.open_project, accelerator="Ctrl+O")
            file_menu.add_command(label="Save project", command=self.save_project_ui, accelerator="Ctrl+S")
            file_menu.add_command(label="Save project as...", command=lambda: self.save_project_ui(True))
            file_menu.add_separator()
            file_menu.add_command(label="Export EPD...", command=self.export_epd_ui)
            file_menu.add_command(label="Export EPC...", command=self.export_epc_ui)
            file_menu.add_command(label="Generate C/H...", command=self.generate_c_ui)
            file_menu.add_separator()
            file_menu.add_command(label="Exit", command=self.destroy)
            menu.add_cascade(label="File", menu=file_menu)

            help_menu = tk.Menu(menu, tearoff=False)
            help_menu.add_command(label="About", command=self.show_about)
            menu.add_cascade(label="Help", menu=help_menu)
            self.config(menu=menu)
            self.bind_all("<Control-n>", lambda _event: self.new_project())
            self.bind_all("<Control-o>", lambda _event: self.open_project())
            self.bind_all("<Control-s>", lambda _event: self.save_project_ui())

        def _create_widgets(self) -> None:
            root = ttk.Frame(self, padding=12)
            root.pack(fill=tk.BOTH, expand=True)

            header = ttk.Frame(root)
            header.pack(fill=tk.X, pady=(0, 10))
            ttk.Label(header, text="AUTOSAR Mem Configurator", style="Title.TLabel").pack(side=tk.LEFT)
            ttk.Button(header, text="Validate", command=self.validate_ui).pack(side=tk.RIGHT, padx=(6, 0))
            ttk.Button(header, text="Generate C/H", command=self.generate_c_ui).pack(side=tk.RIGHT, padx=(6, 0))
            ttk.Button(header, text="Export EPC", command=self.export_epc_ui).pack(side=tk.RIGHT, padx=(6, 0))
            ttk.Button(header, text="Export EPD", command=self.export_epd_ui).pack(side=tk.RIGHT)

            notebook = ttk.Notebook(root)
            notebook.pack(fill=tk.BOTH, expand=True)
            general_tab = ttk.Frame(notebook, padding=14)
            sectors_tab = ttk.Frame(notebook, padding=14)
            output_tab = ttk.Frame(notebook, padding=14)
            notebook.add(general_tab, text="General")
            notebook.add(sectors_tab, text="Flash sectors")
            notebook.add(output_tab, text="Validation / Output")
            self.notebook = notebook

            self._build_general_tab(general_tab)
            self._build_sectors_tab(sectors_tab)
            self._build_output_tab(output_tab)

            status = ttk.Label(root, textvariable=self.status_var, anchor=tk.W, relief=tk.SUNKEN)
            status.pack(fill=tk.X, pady=(10, 0))

        def _build_general_tab(self, tab: ttk.Frame) -> None:
            tab.columnconfigure(1, weight=1)
            metadata_fields = [
                ("Module name", self.module_name_var, "AUTOSAR short name, normally Mem"),
                ("AUTOSAR release", self.release_var, "Project/vendor target, for example R25-11"),
                ("Instance name", self.instance_name_var, "Single instance supported by this driver"),
            ]
            row = 0
            for label, variable, hint in metadata_fields:
                ttk.Label(tab, text=label).grid(row=row, column=0, sticky=tk.W, padx=(0, 12), pady=6)
                ttk.Entry(tab, textvariable=variable).grid(row=row, column=1, sticky=tk.EW, pady=6)
                ttk.Label(tab, text=hint, style="Hint.TLabel").grid(
                    row=row, column=2, sticky=tk.W, padx=(12, 0), pady=6
                )
                row += 1

            parameter_bindings = (
                ("MemDevErrorDetect", self.det_var),
                ("MemVersionInfoApi", self.version_api_var),
                ("MemIndex", self.mem_index_var),
                ("MemInvocation", self.invocation_var),
                ("MemMainFunctionPeriod", self.period_var),
                ("MemStartAddress", self.base_var),
                ("MemSize", self.total_size_var),
                ("MemErasedValue", self.erased_var),
            )
            for parameter_name, variable in parameter_bindings:
                parameter = self.definition.parameters[parameter_name]
                ttk.Label(tab, text=parameter.name).grid(
                    row=row, column=0, sticky=tk.W, padx=(0, 12), pady=6
                )
                if parameter.kind == "BOOLEAN":
                    widget = ttk.Checkbutton(tab, text="Enabled", variable=variable)
                elif parameter.kind == "ENUMERATION":
                    widget = ttk.Combobox(
                        tab,
                        textvariable=variable,
                        values=parameter.literals,
                        state="readonly",
                    )
                else:
                    widget = ttk.Entry(tab, textvariable=variable)
                widget.grid(row=row, column=1, sticky=tk.EW, pady=6)
                ttk.Label(tab, text=self.epd_hint(parameter_name), style="Hint.TLabel").grid(
                    row=row, column=2, sticky=tk.W, padx=(12, 0), pady=6
                )
                row += 1

        def epd_hint(self, parameter_name: str) -> str:
            parameter = self.definition.parameters[parameter_name]
            limits = ""
            if parameter.minimum or parameter.maximum:
                limits = f" [{parameter.minimum or '-inf'}..{parameter.maximum or '+inf'}]"
            return (parameter.description or parameter.name) + limits

        def _build_sectors_tab(self, tab: ttk.Frame) -> None:
            tab.rowconfigure(0, weight=1)
            tab.columnconfigure(0, weight=1)
            columns = ("index", "start", "size", "end")
            self.sector_tree = ttk.Treeview(tab, columns=columns, show="headings", selectmode="browse")
            headings = {
                "index": "#",
                "start": "Start address",
                "size": "Size",
                "end": "End address (exclusive)",
            }
            widths = {"index": 60, "start": 190, "size": 190, "end": 220}
            for column in columns:
                self.sector_tree.heading(column, text=headings[column])
                self.sector_tree.column(column, width=widths[column], anchor=tk.CENTER)
            self.sector_tree.grid(row=0, column=0, columnspan=5, sticky=tk.NSEW)
            scrollbar = ttk.Scrollbar(tab, orient=tk.VERTICAL, command=self.sector_tree.yview)
            scrollbar.grid(row=0, column=5, sticky=tk.NS)
            self.sector_tree.configure(yscrollcommand=scrollbar.set)
            self.sector_tree.bind("<<TreeviewSelect>>", self.select_sector)

            ttk.Label(tab, text="Start address").grid(row=1, column=0, sticky=tk.W, pady=(12, 4))
            ttk.Entry(tab, textvariable=self.sector_start_var).grid(row=2, column=0, sticky=tk.EW, padx=(0, 8))
            ttk.Label(tab, text="Size").grid(row=1, column=1, sticky=tk.W, pady=(12, 4))
            ttk.Entry(tab, textvariable=self.sector_size_var).grid(row=2, column=1, sticky=tk.EW, padx=(0, 8))
            ttk.Button(tab, text="Add", command=self.add_sector).grid(row=2, column=2, padx=4)
            ttk.Button(tab, text="Update", command=self.update_sector).grid(row=2, column=3, padx=4)
            ttk.Button(tab, text="Remove", command=self.remove_sector).grid(row=2, column=4, padx=4)
            ttk.Button(tab, text="Reset STM32F401", command=self.reset_sectors).grid(
                row=3, column=0, sticky=tk.W, pady=(12, 0)
            )
            for column in (0, 1):
                tab.columnconfigure(column, weight=1)

        def _build_output_tab(self, tab: ttk.Frame) -> None:
            tab.rowconfigure(1, weight=1)
            tab.columnconfigure(0, weight=1)
            ttk.Label(
                tab,
                text="Validation errors and generated file paths appear here.",
                style="Hint.TLabel",
            ).grid(row=0, column=0, sticky=tk.W, pady=(0, 8))
            self.output_text = tk.Text(tab, wrap=tk.WORD, font=("Consolas", 10))
            self.output_text.grid(row=1, column=0, sticky=tk.NSEW)
            scrollbar = ttk.Scrollbar(tab, orient=tk.VERTICAL, command=self.output_text.yview)
            scrollbar.grid(row=1, column=1, sticky=tk.NS)
            self.output_text.configure(yscrollcommand=scrollbar.set)

        def log(self, text: str, clear: bool = False) -> None:
            if clear:
                self.output_text.delete("1.0", tk.END)
            self.output_text.insert(tk.END, text.rstrip() + "\n")
            self.output_text.see(tk.END)

        def collect_config(self) -> MemConfig:
            try:
                config = MemConfig(
                    module_name=self.module_name_var.get().strip(),
                    autosar_release=self.release_var.get().strip(),
                    dev_error_detect=self.det_var.get(),
                    version_info_api=self.version_api_var.get(),
                    mem_index=parse_int(self.mem_index_var.get()),
                    invocation=self.invocation_var.get(),
                    main_function_period=float(self.period_var.get().strip()),
                    instance_name=self.instance_name_var.get().strip(),
                    flash_base_address=parse_int(self.base_var.get()),
                    flash_total_size=parse_int(self.total_size_var.get()),
                    erased_value=parse_int(self.erased_var.get()),
                    sectors=[Sector(item.start_address, item.size) for item in self.sectors],
                )
            except ValueError as exc:
                raise ConfigInputError(f"Cannot parse an input value: {exc}") from exc
            return config

        def load_config_into_ui(self, config: MemConfig) -> None:
            self.module_name_var.set(config.module_name)
            self.release_var.set(config.autosar_release)
            self.det_var.set(config.dev_error_detect)
            self.version_api_var.set(config.version_info_api)
            self.mem_index_var.set(str(config.mem_index))
            self.invocation_var.set(config.invocation)
            self.period_var.set(f"{config.main_function_period:.9g}")
            self.instance_name_var.set(config.instance_name)
            self.base_var.set(format_hex(config.flash_base_address))
            self.total_size_var.set(format_hex(config.flash_total_size))
            self.erased_var.set(f"0x{config.erased_value:02X}")
            self.sectors = [Sector(item.start_address, item.size) for item in config.sectors]
            self.refresh_sector_tree()

        def refresh_sector_tree(self) -> None:
            self.sector_tree.delete(*self.sector_tree.get_children())
            for index, sector in enumerate(self.sectors):
                self.sector_tree.insert(
                    "",
                    tk.END,
                    iid=str(index),
                    values=(
                        index,
                        format_hex(sector.start_address),
                        f"{format_hex(sector.size)} ({sector.size // 1024} KiB)",
                        format_hex(sector.end_address),
                    ),
                )

        def selected_sector_index(self) -> int | None:
            selected = self.sector_tree.selection()
            return int(selected[0]) if selected else None

        def select_sector(self, _event: object = None) -> None:
            index = self.selected_sector_index()
            if index is not None:
                sector = self.sectors[index]
                self.sector_start_var.set(format_hex(sector.start_address))
                self.sector_size_var.set(format_hex(sector.size))

        def sector_from_inputs(self) -> Sector:
            try:
                return Sector(
                    parse_int(self.sector_start_var.get()),
                    parse_int(self.sector_size_var.get()),
                )
            except ValueError as exc:
                raise ConfigInputError(f"Invalid sector value: {exc}") from exc

        def add_sector(self) -> None:
            try:
                sector = self.sector_from_inputs()
            except ConfigInputError as exc:
                messagebox.showerror(APP_NAME, str(exc), parent=self)
                return
            self.sectors.append(sector)
            self.refresh_sector_tree()
            self.status_var.set(f"Added sector {len(self.sectors) - 1}")

        def update_sector(self) -> None:
            index = self.selected_sector_index()
            if index is None:
                messagebox.showinfo(APP_NAME, "Select a sector to update.", parent=self)
                return
            try:
                self.sectors[index] = self.sector_from_inputs()
            except ConfigInputError as exc:
                messagebox.showerror(APP_NAME, str(exc), parent=self)
                return
            self.refresh_sector_tree()
            self.status_var.set(f"Updated sector {index}")

        def remove_sector(self) -> None:
            index = self.selected_sector_index()
            if index is None:
                messagebox.showinfo(APP_NAME, "Select a sector to remove.", parent=self)
                return
            del self.sectors[index]
            self.refresh_sector_tree()
            self.status_var.set(f"Removed sector {index}")

        def reset_sectors(self) -> None:
            self.sectors = MemConfig.default().sectors
            self.refresh_sector_tree()
            self.status_var.set("Restored STM32F401RE sector geometry")

        def get_valid_config(self, show_dialog: bool = True) -> MemConfig | None:
            try:
                config = self.collect_config()
                errors = validate_config(config)
                errors.extend(validate_config_against_definition(config, self.definition))
            except ConfigInputError as exc:
                errors = [str(exc)]
                config = None
            if errors:
                self.log("VALIDATION FAILED\n- " + "\n- ".join(errors), clear=True)
                self.notebook.select(2)
                self.status_var.set(f"Validation failed: {len(errors)} error(s)")
                if show_dialog:
                    messagebox.showerror(
                        APP_NAME,
                        "Configuration has errors. See the Validation / Output tab.",
                        parent=self,
                    )
                return None
            return config

        def validate_ui(self) -> None:
            config = self.get_valid_config()
            if config is not None:
                self.log(
                    "VALIDATION PASSED\n"
                    f"- EPD definition: {self.definition.source_path}\n"
                    f"- Flash range: {format_hex(config.flash_base_address)} .. "
                    f"{format_hex(config.flash_base_address + config.flash_total_size - 1)}\n"
                    f"- Total size: {config.flash_total_size // 1024} KiB\n"
                    f"- Sector count: {len(config.sectors)}",
                    clear=True,
                )
                self.notebook.select(2)
                self.status_var.set("Configuration is valid")

        def new_project(self) -> None:
            self.project_path = None
            self.load_config_into_ui(load_configuration(DEFAULT_EPC_PATH))
            self.log("New default STM32F401RE project.", clear=True)
            self.status_var.set("New project")

        def open_project(self) -> None:
            filename = filedialog.askopenfilename(
                parent=self,
                title="Open or import Mem configuration",
                filetypes=(
                    ("Supported configuration", "*.memcfg.json *.epc *.epd *.xml"),
                    ("Mem configuration", "*.memcfg.json"),
                    ("AUTOSAR / legacy XML", "*.epc *.epd *.xml"),
                    ("All files", "*.*"),
                ),
            )
            if not filename:
                return
            path = Path(filename)
            try:
                config = load_configuration(path)
            except ConfigInputError as exc:
                messagebox.showerror(APP_NAME, str(exc), parent=self)
                return
            imported_xml = path.suffix.lower() != ".json"
            self.project_path = None if imported_xml else path
            self.load_config_into_ui(config)
            action = "Imported" if imported_xml else "Opened"
            self.log(
                f"{action} configuration:\n{filename}"
                + ("\nSave as .memcfg.json to keep edits in the integrated format." if imported_xml else ""),
                clear=True,
            )
            self.status_var.set(f"{action} {path.name}")

        def save_project_ui(self, save_as: bool = False) -> None:
            try:
                config = self.collect_config()
            except ConfigInputError as exc:
                messagebox.showerror(APP_NAME, str(exc), parent=self)
                return
            path = self.project_path
            if save_as or path is None:
                filename = filedialog.asksaveasfilename(
                    parent=self,
                    title="Save Mem configuration project",
                    defaultextension=".memcfg.json",
                    filetypes=(("Mem configuration", "*.memcfg.json"), ("JSON", "*.json")),
                    initialfile="Mem.memcfg.json",
                )
                if not filename:
                    return
                path = Path(filename)
            try:
                save_project(config, path)
            except OSError as exc:
                messagebox.showerror(APP_NAME, f"Cannot save project: {exc}", parent=self)
                return
            self.project_path = path
            self.log(f"Saved project:\n{path}", clear=True)
            self.status_var.set(f"Saved {path.name}")

        def export_epd_ui(self) -> None:
            filename = filedialog.asksaveasfilename(
                parent=self,
                title="Export EPD parameter definition",
                defaultextension=".epd",
                filetypes=(("AUTOSAR EPD", "*.epd"), ("XML", "*.xml")),
                initialfile="Mem.epd",
            )
            if not filename:
                return
            try:
                write_xml(ET.parse(DEFAULT_EPD_PATH), Path(filename))
            except (OSError, ET.ParseError) as exc:
                messagebox.showerror(APP_NAME, f"Cannot export EPD: {exc}", parent=self)
                return
            self.log(f"EPD exported:\n{filename}", clear=True)
            self.status_var.set("EPD export completed")

        def export_epc_ui(self) -> None:
            config = self.get_valid_config()
            if config is None:
                return
            filename = filedialog.asksaveasfilename(
                parent=self,
                title="Export EPC configuration values",
                defaultextension=".epc",
                filetypes=(("AUTOSAR EPC", "*.epc"), ("XML", "*.xml")),
                initialfile="Mem.epc",
            )
            if not filename:
                return
            try:
                write_xml(create_epc_tree(config), Path(filename))
            except (OSError, ConfigInputError) as exc:
                messagebox.showerror(APP_NAME, f"Cannot export EPC: {exc}", parent=self)
                return
            self.log(f"EPC exported:\n{filename}", clear=True)
            self.status_var.set("EPC export completed")

        def generate_c_ui(self) -> None:
            config = self.get_valid_config()
            if config is None:
                return
            dirname = filedialog.askdirectory(
                parent=self,
                title="Select output root (include/ and src/ will be created)",
                initialdir=str(DEFAULT_OUTPUT_ROOT),
            )
            if not dirname:
                return
            output_root = Path(dirname)
            existing = [path for path in generated_c_paths(output_root) if path.exists()]
            if existing and not messagebox.askyesno(
                APP_NAME,
                "The following generated files already exist and will be overwritten:\n\n"
                + "\n".join(str(path) for path in existing)
                + "\n\nContinue?",
                parent=self,
            ):
                return
            try:
                paths = write_c_files(
                    config,
                    output_root,
                    self.definition.source_path,
                    DEFAULT_TEMPLATE_ROOT,
                )
            except (OSError, ConfigInputError) as exc:
                messagebox.showerror(APP_NAME, f"Cannot generate C/H files: {exc}", parent=self)
                return
            self.log("C/H generation completed:\n" + "\n".join(str(path) for path in paths), clear=True)
            self.notebook.select(2)
            self.status_var.set("C/H generation completed")

        def show_about(self) -> None:
            messagebox.showinfo(
                APP_NAME,
                f"{APP_NAME} {APP_VERSION}\n\n"
                "Single-file AUTOSAR Mem/Flash configuration tool.\n"
                "Imports native JSON, standard AUTOSAR EPC, and the legacy\n"
                "EPD/EPC format from the gui-code-generation branch.\n"
                "Uses only the Python standard library.\n\n"
                "Validate EPD/EPC with the exact vendor XSD/plugin before production use.",
                parent=self,
            )


def export_all(
    config: MemConfig,
    output_root: Path,
    definition_path: Path = DEFAULT_EPD_PATH,
    template_root: Path = DEFAULT_TEMPLATE_ROOT,
) -> list[Path]:
    definition = load_module_definition(definition_path)
    require_valid(config, definition)
    output_root.mkdir(parents=True, exist_ok=True)
    project_path = output_root / "Mem.memcfg.json"
    epd_path = output_root / "Mem.epd"
    epc_path = output_root / "Mem.epc"
    save_project(config, project_path)
    write_xml(ET.parse(definition_path), epd_path)
    write_xml(create_epc_tree(config), epc_path)
    return [
        project_path,
        epd_path,
        epc_path,
        *write_c_files(config, output_root, definition_path, template_root),
    ]


def run_self_test(
    definition_path: Path = DEFAULT_EPD_PATH,
    template_root: Path = DEFAULT_TEMPLATE_ROOT,
) -> int:
    config = MemConfig.default()
    try:
        definition = load_module_definition(definition_path)
    except ConfigInputError as exc:
        print(exc, file=sys.stderr)
        return 1
    errors = validate_config(config)
    errors.extend(validate_config_against_definition(config, definition))
    if errors:
        print("Default configuration validation failed:", file=sys.stderr)
        print("\n".join(errors), file=sys.stderr)
        return 1
    with tempfile.TemporaryDirectory(prefix="mem_configurator_") as directory:
        root = Path(directory)
        canonical_epc = load_configuration(DEFAULT_EPC_PATH)
        if canonical_epc != config:
            print("Canonical STM32F401 EPC does not match default configuration.", file=sys.stderr)
            return 1

        serialized_epd_path = root / "SerializedMem.epd"
        write_xml(create_epd_tree(), serialized_epd_path)
        serialized_definition = load_module_definition(serialized_epd_path)
        canonical_signature = {
            name: (item.container, item.kind, item.literals)
            for name, item in definition.parameters.items()
        }
        serialized_signature = {
            name: (item.container, item.kind, item.literals)
            for name, item in serialized_definition.parameters.items()
        }
        if serialized_signature != canonical_signature:
            print("Canonical EPD and built-in EPD serializer are out of sync.", file=sys.stderr)
            return 1

        paths = export_all(config, root, definition_path, template_root)
        for path in paths:
            if not path.is_file() or path.stat().st_size == 0:
                print(f"Missing or empty generated file: {path}", file=sys.stderr)
                return 1
        ET.parse(root / "Mem.epd")
        ET.parse(root / "Mem.epc")
        loaded = load_project(root / "Mem.memcfg.json")
        if loaded != config:
            print("Project round-trip mismatch.", file=sys.stderr)
            return 1
        loaded_epc = load_configuration(root / "Mem.epc")
        if loaded_epc != config:
            print("AUTOSAR EPC import round-trip mismatch.", file=sys.stderr)
            return 1

        epc_variant = MemConfig.default()
        epc_variant.dev_error_detect = False
        epc_variant.version_info_api = False
        epc_variant.mem_index = 7
        epc_variant.main_function_period = 0.0125
        epc_variant.instance_name = "ImportedInstance"
        epc_variant.erased_value = 0xA5
        variant_path = root / "Variant.epc"
        write_xml(create_epc_tree(epc_variant), variant_path)
        if load_configuration(variant_path) != epc_variant:
            print("Non-default AUTOSAR EPC import round-trip mismatch.", file=sys.stderr)
            return 1

        legacy_path = root / "MemberBranch.epd"
        legacy_path.write_text(
            """<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR_EPD version="4.4.0">
  <Module name="MemDriver">
    <Container name="MemGeneral">
      <Parameter name="MemDevErrorDetect" type="BOOLEAN" default="false"/>
      <Parameter name="MemIndex" type="INTEGER" default="3"/>
      <Parameter name="MemErasedValue" type="HEX" default="0xFF"/>
    </Container>
    <Container name="MemInstance">
      <List name="MemSectorBatch">
        <Item MemStartAddress="0x08000000" MemEraseSectorSize="16384" MemNumberOfSectors="2"/>
      </List>
    </Container>
  </Module>
</AUTOSAR_EPD>
""",
            encoding="utf-8",
        )
        legacy = load_configuration(legacy_path)
        if (
            legacy.dev_error_detect
            or legacy.mem_index != 3
            or legacy.flash_total_size != 0x8000
            or len(legacy.sectors) != 2
            or validate_config(legacy)
        ):
            print("Legacy member-branch EPD import failed.", file=sys.stderr)
            return 1
        if "FLASH_IP_SECTOR_COUNT       8u" not in (root / "include" / "Flash_IP_Cfg.h").read_text(encoding="utf-8"):
            print("Generated Flash sector count is incorrect.", file=sys.stderr)
            return 1
        for generated_path in generated_c_paths(root):
            generated_text = generated_path.read_text(encoding="utf-8")
            if TEMPLATE_TOKEN_RE.search(generated_text):
                print(f"Unresolved template token in {generated_path}.", file=sys.stderr)
                return 1
        flash_source = (root / "src" / "Flash_IP_Cfg.c").read_text(encoding="utf-8")
        total_macro = flash_source.split(
            "#define FLASH_IP_CFG_SECTOR_TOTAL_SIZE_LITERAL \\\n", 1
        )[1].split("\n\n", 1)[0]
        if total_macro.count("(") != total_macro.count(")"):
            print("Generated Flash size-sum macro has unbalanced parentheses.", file=sys.stderr)
            return 1
        invalid = MemConfig.default()
        invalid.sectors[1].start_address += 1
        if not validate_config(invalid):
            print("Validator did not detect an invalid sector gap.", file=sys.stderr)
            return 1
        print(f"Self-test passed. Generated and checked {len(paths)} files.")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="run headless model/XML/code-generation checks",
    )
    parser.add_argument(
        "--export-default",
        metavar="DIRECTORY",
        help="export the default STM32F401RE project, EPD/EPC and C/H files",
    )
    parser.add_argument(
        "--generate",
        metavar="CONFIGURATION",
        help="load native JSON, AUTOSAR EPC, or legacy EPD/EPC and export all artifacts",
    )
    parser.add_argument(
        "-o",
        "--output",
        metavar="DIRECTORY",
        help="output directory used together with --generate",
    )
    parser.add_argument(
        "--definition",
        metavar="EPD",
        default=str(DEFAULT_EPD_PATH),
        help="canonical AUTOSAR EPD used for metadata and validation",
    )
    parser.add_argument(
        "--templates",
        metavar="DIRECTORY",
        default=str(DEFAULT_TEMPLATE_ROOT),
        help="root containing include/ and src/ code templates",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.self_test:
        return run_self_test(Path(args.definition), Path(args.templates))
    if args.export_default:
        paths = export_all(
            MemConfig.default(),
            Path(args.export_default),
            Path(args.definition),
            Path(args.templates),
        )
        print("\n".join(str(path) for path in paths))
        return 0
    if args.generate:
        if not args.output:
            print("--generate requires --output DIRECTORY", file=sys.stderr)
            return 2
        try:
            paths = export_all(
                load_configuration(Path(args.generate)),
                Path(args.output),
                Path(args.definition),
                Path(args.templates),
            )
        except ConfigInputError as exc:
            print(exc, file=sys.stderr)
            return 1
        print("\n".join(str(path) for path in paths))
        return 0
    if args.output:
        print("--output is only valid with --generate", file=sys.stderr)
        return 2
    if tk is None:
        print("Tkinter is not available. Use --self-test or install Tk support.", file=sys.stderr)
        return 1
    app = MemConfiguratorApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
