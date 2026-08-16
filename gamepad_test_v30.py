#!/usr/bin/env python3
"""ClawBridge v30 手柄测试 — 虚拟手柄 + 摇杆"""

import socket
import time
import sys

HOST = "127.0.0.1"
PORT = 9999

BUTTONS = {
    "A":          0x1000, "B": 0x2000, "X": 0x4000, "Y": 0x8000,
    "DPAD_UP":    0x0001, "DPAD_DOWN": 0x0002, "DPAD_LEFT": 0x0004, "DPAD_RIGHT": 0x0008,
    "BACK":       0x0020, "START":      0x0010,
    "LB":         0x0100, "RB":         0x0200,
    "LS":         0x0040, "RS":         0x0080,
}

def send_cmd(cmd):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3)
        s.connect((HOST, PORT))
        s.sendall((cmd + "\n").encode("utf-8"))
        s.close()
    except Exception as e:
        print(f"  ERROR: {e}")
        sys.exit(1)

def btn_tap(name):
    val = BUTTONS[name]
    send_cmd(f"GP:{val}_PRESS")
    time.sleep(0.08)
    send_cmd(f"GP:{val}_RELEASE")
    time.sleep(0.08)

def main():
    if len(sys.argv) > 1:
        name = sys.argv[1].upper()
        if name in BUTTONS:
            print(f"Tap: {name} (0x{BUTTONS[name]:04X})")
            btn_tap(name)
        else:
            print(f"Unknown: {name}")
            print(f"Available: {', '.join(BUTTONS.keys())}")
        return

    print("\n" + "=" * 50)
    print("  Gamepad Test - ClawBridge v30")
    print("=" * 50)
    print(f"  Target: {HOST}:{PORT}")
    print(f"  Open https://gamepad-tester.com on remote PC")
    print("=" * 50)

    # Buttons
    print("\n--- Buttons ---")
    for name, val in BUTTONS.items():
        print(f"  [{name:12s} 0x{val:04X}]", end="", flush=True)
        btn_tap(name)
        time.sleep(1.4)

    # Joystick
    print("\n--- Joystick ---")
    tests = [
        ("LS center",    "Joy:0,0,0,0"),
        ("LS top-left",  "Joy:-32768,-32768,0,0"),
        ("LS bottom-right", "Joy:32767,32767,0,0"),
        ("RS top-right", "Joy:0,0,32767,-32768"),
        ("RS bottom-left","Joy:0,0,-32768,32767"),
        ("both sticks",  "Joy:-32768,0,32767,0"),
    ]
    for desc, cmd in tests:
        print(f"  {desc}: {cmd}", flush=True)
        send_cmd(cmd)
        time.sleep(2)
    send_cmd("Joy:0,0,0,0")
    print("  JS reset")
    time.sleep(1)

    # Triggers
    print("\n--- Triggers ---")
    trig_tests = [
        ("LT 50%",    "Trig:128,0"),
        ("LT 100%",   "Trig:255,0"),
        ("RT 50%",    "Trig:0,128"),
        ("RT 100%",   "Trig:0,255"),
        ("both 100%", "Trig:255,255"),
    ]
    for desc, cmd in trig_tests:
        print(f"  {desc}: {cmd}", flush=True)
        send_cmd(cmd)
        time.sleep(2)
    send_cmd("Trig:0,0")
    print("  Trig reset")

    print("\n" + "=" * 50)
    print("  Done!")
    print("=" * 50)

if __name__ == "__main__":
    main()
