#!/usr/bin/env python3
"""
ESP32PLC config JSON generator
Generates WiFiconfig.json and/or MQTTconfig.json ready to upload to the device.

Usage:
    python tools/make_config.py wifi            # interactive WiFi config
    python tools/make_config.py mqtt            # interactive MQTT config
    python tools/make_config.py all             # both, interactive
    python tools/make_config.py wifi --ssid MyNet --mode sta
    python tools/make_config.py all --out ./configs
"""

import argparse
import getpass
import ipaddress
import json
import os
import re
import sys

# ── output directory (default: data/ relative to this script) ────────────────
SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
DEFAULT_OUT = os.path.join(SCRIPT_DIR, "..", "data")


# ── helpers ──────────────────────────────────────────────────────────────────

def prompt(label, default="", secret=False, validator=None):
    """Prompt the user for a value, showing the default. Returns stripped string."""
    hint = f" [{default}]" if default != "" else ""
    while True:
        if secret:
            val = getpass.getpass(f"  {label}{hint}: ")
        else:
            val = input(f"  {label}{hint}: ").strip()

        if val == "" and default != "":
            val = default
        elif val == "":
            val = ""

        if validator:
            err = validator(val)
            if err:
                print(f"    ! {err}")
                continue
        return val


def prompt_bool(label, default=False):
    hint = "Y/n" if default else "y/N"
    while True:
        val = input(f"  {label} [{hint}]: ").strip().lower()
        if val == "":
            return default
        if val in ("y", "yes", "1", "true"):
            return True
        if val in ("n", "no", "0", "false"):
            return False
        print("    ! Enter y or n")


def prompt_int(label, default, lo=1, hi=65535):
    while True:
        raw = input(f"  {label} [{default}]: ").strip()
        if raw == "":
            return default
        try:
            v = int(raw)
            if lo <= v <= hi:
                return v
            print(f"    ! Must be {lo}–{hi}")
        except ValueError:
            print("    ! Must be a number")


def validate_ip(val):
    if val == "":
        return None
    try:
        ipaddress.ip_address(val)
        return None
    except ValueError:
        return f"'{val}' is not a valid IP address"


def write_json(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"\n  Saved: {path}")


def print_summary(label, data):
    print(f"\n  ── {label} ──")
    for k, v in data.items():
        display = "****" if "pass" in k.lower() or "Pass" in k or "password" in k.lower() else v
        print(f"    {k}: {display}")


# ── WiFi config ───────────────────────────────────────────────────────────────

def make_wifi(args, out_dir):
    print("\n=== WiFi Config ===")

    # mode
    if args.mode:
        mode_str = args.mode.lower()
    else:
        mode_str = prompt("Mode", default="ap", validator=lambda v:
            None if v.lower() in ("ap", "sta") else "Enter 'ap' or 'sta'")
    mode = 1 if mode_str.lower() == "sta" else 2

    # ssid / password (only meaningful in STA mode but still stored)
    if args.ssid is not None:
        ssid = args.ssid
    else:
        ssid = prompt("SSID", default="" if mode == 2 else "",
                      validator=lambda v: "SSID required for STA mode"
                      if mode == 1 and v == "" else None)

    if args.password is not None:
        password = args.password
    else:
        password = prompt("Password", default="", secret=True)

    if args.host is not None:
        host = args.host
    else:
        host = prompt("Hostname", default="",
                      validator=lambda v: "Max 39 characters" if len(v) > 39 else None)

    data = {
        "WIFIMode":      mode,
        "SSID":          ssid,
        "Passcode":      password,
        "Host":          host,       # empty = auto-generated on device
        "DHCP":          1,
        "IP":            "192.168.4.1",
        "DefultGateway": "0.0.0.0",
        "SubMask":       "255.255.255.0",
    }

    print_summary("WiFi config", data)
    out_path = os.path.join(out_dir, "WiFiconfig.json")
    write_json(out_path, data)
    return data


# ── MQTT config ───────────────────────────────────────────────────────────────

def make_mqtt(args, out_dir):
    print("\n=== MQTT Config ===")

    if args.mqtt_enable is not None:
        enabled = args.mqtt_enable
    else:
        enabled = prompt_bool("Enable MQTT", default=False)

    if args.mqtt_ip is not None:
        ip = args.mqtt_ip
    else:
        ip = prompt("Broker IP", default="",
                    validator=lambda v: validate_ip(v) if v else
                    ("IP required when MQTT is enabled" if enabled else None))

    port = prompt_int("Port", default=1883) if args.mqtt_port is None else args.mqtt_port

    if args.mqtt_user is not None:
        user = args.mqtt_user
    else:
        user = prompt("Username", default="esp32plc")

    if args.mqtt_password is not None:
        password = args.mqtt_password
    else:
        password = prompt("Password", default="", secret=True)

    data = {
        "MQTTEnabble":    1 if enabled else 0,
        "MQTTIP":         ip,
        "MQTTUser":       user,
        "MQTTPassword":   password,
        "MQTTPort":       port,
    }

    print_summary("MQTT config", data)
    out_path = os.path.join(out_dir, "MQTTconfig.json")
    write_json(out_path, data)
    return data


# ── CLI ───────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Generate ESP32PLC config JSON files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python tools/make_config.py wifi
  python tools/make_config.py mqtt --mqtt-ip 192.168.1.10 --mqtt-user admin
  python tools/make_config.py all  --out ./configs
  python tools/make_config.py wifi --mode sta --ssid MyNet
        """)

    parser.add_argument("target", choices=["wifi", "mqtt", "all"],
                        help="Which config file(s) to generate")
    parser.add_argument("--out", default=DEFAULT_OUT, metavar="DIR",
                        help=f"Output directory (default: {DEFAULT_OUT})")

    # WiFi args
    wifi = parser.add_argument_group("WiFi options")
    wifi.add_argument("--mode",     choices=["ap", "sta"], help="WiFi mode")
    wifi.add_argument("--ssid",     help="Network SSID (STA mode)")
    wifi.add_argument("--password", help="WiFi password")
    wifi.add_argument("--host",     help="Hostname (blank = auto-generated on device)")

    # MQTT args
    mqtt = parser.add_argument_group("MQTT options")
    mqtt.add_argument("--mqtt-enable",   action="store_true", default=None,
                      dest="mqtt_enable", help="Enable MQTT")
    mqtt.add_argument("--mqtt-ip",       dest="mqtt_ip",       help="Broker IP address")
    mqtt.add_argument("--mqtt-port",     dest="mqtt_port",     type=int, help="Broker port (default 1883)")
    mqtt.add_argument("--mqtt-user",     dest="mqtt_user",     help="MQTT username")
    mqtt.add_argument("--mqtt-password", dest="mqtt_password", help="MQTT password")

    args = parser.parse_args()
    out_dir = os.path.abspath(args.out)

    print(f"Output directory: {out_dir}")

    try:
        if args.target in ("wifi", "all"):
            make_wifi(args, out_dir)
        if args.target in ("mqtt", "all"):
            make_mqtt(args, out_dir)
    except KeyboardInterrupt:
        print("\n\nCancelled.")
        sys.exit(1)

    print("\nDone. Upload to device via the web portal (WiFi or MQTT tab → Load JSON).")


if __name__ == "__main__":
    main()
