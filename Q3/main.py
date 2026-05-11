"""
main.py  –  Entry point for GET HARDWARE INFO assignment.

Detects the current OS, instantiates the correct HostInfo subclass,
calls get_hardware_info(), and displays the result in JSON format.

Usage:
    python main.py
"""

import platform
from host_info import LinuxHost, WindowsHost


def main():
    current_os = platform.system()          # 'Linux', 'Windows', 'Darwin', …

    if current_os == "Linux":
        host = LinuxHost()
    elif current_os == "Windows":
        host = WindowsHost()
    else:
        # macOS falls back to LinuxHost because it supports the same POSIX commands
        print(f"[Info] OS '{current_os}' detected – using LinuxHost (POSIX fallback).")
        host = LinuxHost()

    host.get_hardware_info()
    host.display_hardware_info()


if __name__ == "__main__":
    main()
