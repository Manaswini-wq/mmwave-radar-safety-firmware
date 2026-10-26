#!/usr/bin/env python3
"""
flash_jtag.py - JTAG Flash Utility for AWR2944

Supports flashing firmware via:
  - TI XDS200/XDS560v2 using OpenOCD
  - Lauterbach TRACE32

Usage:
    python flash_jtag.py --interface xds200 --binary build/awr2944_fw.bin
    python flash_jtag.py --interface lauterbach --binary build/awr2944_fw.bin
    python flash_jtag.py --verify --binary build/awr2944_fw.bin
"""

import argparse
import os
import subprocess
import sys
import time


# Target configuration
TARGET_FLASH_BASE = 0x20000000   # QSPI flash base address
TARGET_RAM_BASE   = 0x10200000   # L2 RAM base for RAM loading
TARGET_TCMA_BASE  = 0x00000000   # TCMA for vector table

# OpenOCD configuration
OPENOCD_CFG = """
# AWR2944 OpenOCD configuration
source [find interface/ti-xds.cfg]
transport select jtag

set _CHIPNAME awr2944
set _DAP_TAPID 0x0BB11477

jtag newtap $_CHIPNAME cpu -irlen 4 -expected-id $_DAP_TAPID
dap create $_CHIPNAME.dap -chain-position $_CHIPNAME.cpu
target create $_CHIPNAME.r5f cortex_r4 -dap $_CHIPNAME.dap -ap-num 0

$_CHIPNAME.r5f configure -work-area-phys 0x10200000 -work-area-size 0x4000 -work-area-backup 0

adapter speed 2500
"""


def flash_openocd(binary_path: str, interface: str, verify: bool = False):
    """Flash firmware using OpenOCD."""
    print(f"Flashing {binary_path} via OpenOCD ({interface})...")

    # Write OpenOCD config to temp file
    cfg_path = "/tmp/awr2944_openocd.cfg"
    with open(cfg_path, "w") as f:
        f.write(OPENOCD_CFG)

    # Build OpenOCD command
    cmd = [
        "openocd",
        "-f", cfg_path,
        "-c", f"init; halt; load_image {binary_path} {TARGET_FLASH_BASE:#x} bin",
    ]

    if verify:
        cmd.extend(["-c", f"verify_image {binary_path} {TARGET_FLASH_BASE:#x} bin"])

    cmd.extend(["-c", "reset run; shutdown"])

    print(f"  Command: {' '.join(cmd)}")

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        if result.returncode == 0:
            print("  Flash complete!")
        else:
            print(f"  ERROR: {result.stderr}")
            sys.exit(1)
    except FileNotFoundError:
        print("  ERROR: OpenOCD not found. Install with: sudo apt install openocd")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print("  ERROR: Flash operation timed out")
        sys.exit(1)


def flash_lauterbach(binary_path: str, verify: bool = False):
    """Flash firmware using Lauterbach TRACE32."""
    print(f"Flashing {binary_path} via Lauterbach TRACE32...")

    # Generate TRACE32 command script
    cmm_script = f"""
; AWR2944 Flash Script (auto-generated)
SYStem.RESet
SYStem.CPU CortexR5
SYStem.CONFIG.DEBUGPORTTYPE JTAG
SYStem.Up

; Load binary to flash
FLASH.ReProgram ALL
Data.LOAD.Binary {binary_path} {TARGET_FLASH_BASE:#x}
FLASH.ReProgram OFF
"""

    if verify:
        cmm_script += f"""
; Verify
Data.VERIFY.Binary {binary_path} {TARGET_FLASH_BASE:#x}
"""

    cmm_script += """
; Reset and run
SYStem.Down
SYStem.Up
Go
ENDDO
"""

    cmm_path = "/tmp/awr2944_flash.cmm"
    with open(cmm_path, "w") as f:
        f.write(cmm_script)

    # Launch TRACE32
    t32_exe = os.environ.get("T32_PATH", "/opt/t32/bin/pc_linux64/t32marm")

    try:
        cmd = [t32_exe, "-c", "config.t32", "-s", cmm_path]
        print(f"  Command: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        if result.returncode == 0:
            print("  Flash complete!")
        else:
            print(f"  ERROR: {result.stderr}")
    except FileNotFoundError:
        print(f"  ERROR: TRACE32 not found at {t32_exe}")
        print("  Set T32_PATH environment variable to TRACE32 executable path")
        sys.exit(1)


def load_to_ram(binary_path: str):
    """Load firmware directly to RAM (for development/debug)."""
    print(f"Loading {binary_path} to RAM at {TARGET_RAM_BASE:#x}...")

    cmd = [
        "openocd",
        "-f", "/tmp/awr2944_openocd.cfg",
        "-c", f"init; halt; load_image {binary_path} {TARGET_RAM_BASE:#x} bin",
        "-c", f"reg pc {TARGET_TCMA_BASE:#x}; resume; shutdown",
    ]

    subprocess.run(cmd, capture_output=True, text=True)
    print("  RAM load complete!")


def main():
    parser = argparse.ArgumentParser(description="AWR2944 JTAG Flash Utility")
    parser.add_argument("--interface", required=True, choices=["xds200", "xds560", "lauterbach", "ram"])
    parser.add_argument("--binary", required=True, help="Path to firmware binary")
    parser.add_argument("--verify", action="store_true", help="Verify after flashing")
    parser.add_argument("--address", type=lambda x: int(x, 0), default=TARGET_FLASH_BASE,
                        help="Flash target address")
    args = parser.parse_args()

    if not os.path.isfile(args.binary):
        print(f"ERROR: Binary file not found: {args.binary}")
        sys.exit(1)

    file_size = os.path.getsize(args.binary)
    print(f"Firmware: {args.binary} ({file_size} bytes)")
    print(f"Target address: {args.address:#010x}")

    if args.interface in ("xds200", "xds560"):
        flash_openocd(args.binary, args.interface, args.verify)
    elif args.interface == "lauterbach":
        flash_lauterbach(args.binary, args.verify)
    elif args.interface == "ram":
        load_to_ram(args.binary)


if __name__ == "__main__":
    main()
