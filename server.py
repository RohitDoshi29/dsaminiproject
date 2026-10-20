#!/usr/bin/env python3
"""
Courier Parcel Tracking & Dispatch - Python Launcher
Launches the native C++ web server or provides a fallback launcher.
Usage:
    python3 server.py [port]
"""

import sys
import os
import subprocess

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "5000"
    base_dir = os.path.dirname(os.path.abspath(__file__))
    binary = os.path.join(base_dir, "bin", "courier_tracker")

    if not os.path.exists(binary):
        print(f"[!] Binary '{binary}' not found. Compiling via make...")
        subprocess.run(["make", "-C", base_dir], check=True)

    print(f">>> Launching Courier Parcel Tracking Web System on http://localhost:{port}/ ...")
    cmd = [binary, "--web", port]
    try:
        subprocess.run(cmd)
    except KeyboardInterrupt:
        print("\nServer stopped.")

if __name__ == "__main__":
    main()

