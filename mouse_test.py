#!/usr/bin/env python3
"""ClawBridge 鼠标测试 — 按键+移动"""

import socket
import time
import sys

HOST = "127.0.0.1"
PORT = 9999

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

def click(btn, label):
    print(f"  {label}...", end="", flush=True)
    send_cmd(f"MB:{btn}_PRESS")
    time.sleep(0.05)
    send_cmd(f"MB:{btn}_RELEASE")
    time.sleep(0.1)
    print(" sent")

def move(dx, dy, label):
    print(f"  {label} (MV:{dx},{dy})...", end="", flush=True)
    send_cmd(f"MV:{dx},{dy}")
    time.sleep(0.1)
    print(" sent")

def main():
    print("\n" + "=" * 40)
    print("  🖱️  Mouse Test")
    print("=" * 40)
    print(f"  Target: {HOST}:{PORT}")
    print("=" * 40)

    if len(sys.argv) > 1:
        name = sys.argv[1].lower()
        if name == "left":
            click(1, "Left click")
        elif name == "right":
            click(3, "Right click")
        elif name == "middle":
            click(2, "Middle click")
        elif name == "all":
            print("\n--- Clicks ---")
            click(1, "Left")
            time.sleep(0.5)
            click(3, "Right")
            print("\n--- Movement ---")
            move(100, 0, "Move right")
            time.sleep(0.5)
            move(-100, 0, "Move left")
            time.sleep(0.5)
            move(0, 50, "Move down")
        elif name == "move":
            print("\n--- Mouse Movement ---")
            move(100, 0, "Right 100")
            time.sleep(0.5)
            move(-100, 0, "Left 100")
            time.sleep(0.5)
            move(0, 100, "Down 100")
            time.sleep(0.5)
            move(0, -100, "Up 100")
            time.sleep(0.5)
            move(100, 100, "Diagonal ↘")
            time.sleep(0.5)
            move(-100, -100, "Diagonal ↖")
        else:
            print(f"Usage: {sys.argv[0]} [left|right|middle|move|all]")
        return

    # Interactive
    while True:
        print("\n  [1] Left click   [3] Right click")
        print("  [M] Move test    [Q] Quit")
        choice = input("  Choose: ").strip().lower()
        if choice == "1":
            click(1, "Left click")
        elif choice == "3":
            click(3, "Right click")
        elif choice in ("m", "move"):
            print("\n  Moving...")
            for d, label in [(100,0,"→"), (-100,0,"←"), (0,50,"↓"), (0,-50,"↑")]:
                move(d[0], d[1], f"Move {label}")
                time.sleep(0.3)
        elif choice == "q":
            break
        time.sleep(0.3)

if __name__ == "__main__":
    main()
