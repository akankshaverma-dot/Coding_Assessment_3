"""
host_info.py  –  Module containing HostInfo base class and subclasses.

GET HARDWARE INFO - Python Coding Assignment
"""

import abc
import json
import os
import platform
import subprocess


# ============================================================
# Abstract base class
# ============================================================
class HostInfo(abc.ABC):
    """
    Abstract parent class for host hardware information.
    Child classes: LinuxHost, WindowsHost
    """

    def __init__(self):
        self.hostname:   str = ""
        self.memory:     str = ""
        self.cpu:        str = ""
        self.ip:         str = ""
        self.disk_size:  str = ""

    @abc.abstractmethod
    def get_hardware_info(self) -> None:
        """
        Query OS-level commands and populate the instance attributes.
        Must be implemented by each subclass.
        """
        ...

    def display_hardware_info(self) -> None:
        """Display the fetched hardware info as a formatted JSON string."""
        data = {
            "hostname":  self.hostname,
            "memory":    self.memory,
            "cpu":       self.cpu,
            "ip":        self.ip,
            "disk_size": self.disk_size,
        }
        print(json.dumps(data, indent=4))

    # ----------------------------------------------------------
    # Shared helper: run a shell command and return stdout text
    # ----------------------------------------------------------
    @staticmethod
    def _run(cmd: str, shell: bool = True) -> str:
        try:
            result = subprocess.run(
                cmd,
                shell=shell,
                capture_output=True,
                text=True,
                timeout=10,
            )
            return result.stdout.strip()
        except Exception:
            return ""


# ============================================================
# Linux implementation
# ============================================================
class LinuxHost(HostInfo):
    """Fetches hardware info on a Linux system using lshw / standard commands."""

    def get_hardware_info(self) -> None:
        # Hostname
        self.hostname = self._run("hostname")

        # IP address (first non-loopback)
        ip_raw = self._run(
            "hostname -I 2>/dev/null || ip route get 1 | awk '{print $7; exit}'"
        )
        self.ip = ip_raw.split()[0] if ip_raw else "N/A"

        # Total RAM
        mem_raw = self._run(
            "grep MemTotal /proc/meminfo | awk '{print $2}'"
        )
        if mem_raw:
            mem_mb = int(mem_raw) // 1024
            self.memory = f"{mem_mb} MB"
        else:
            self.memory = "N/A"

        # CPU model
        cpu_raw = self._run(
            "lscpu | grep 'Model name' | sed 's/Model name:\\s*//' | head -1"
        )
        self.cpu = cpu_raw if cpu_raw else self._run(
            "cat /proc/cpuinfo | grep 'model name' | head -1 | cut -d: -f2"
        ).strip()

        # Disk size (total of all block devices)
        disk_raw = self._run(
            "df -h --total 2>/dev/null | tail -1 | awk '{print $2}'"
        )
        self.disk_size = disk_raw if disk_raw else "N/A"


# ============================================================
# Windows implementation
# ============================================================
class WindowsHost(HostInfo):
    """Fetches hardware info on a Windows system using systeminfo / wmic."""

    def get_hardware_info(self) -> None:
        # Hostname
        self.hostname = self._run("hostname")

        # IP address
        ip_raw = self._run(
            'powershell -Command '
            '"(Get-NetIPAddress -AddressFamily IPv4 | '
            'Where-Object { $_.IPAddress -notlike \'127.*\' } | '
            'Select-Object -First 1).IPAddress"'
        )
        self.ip = ip_raw if ip_raw else self._run("ipconfig | findstr /i 'IPv4'").split(":")[-1].strip()

        # Total RAM via wmic
        mem_raw = self._run(
            "wmic ComputerSystem get TotalPhysicalMemory /Value"
        )
        for line in mem_raw.splitlines():
            if "TotalPhysicalMemory" in line:
                bytes_val = line.split("=")[-1].strip()
                if bytes_val.isdigit():
                    mem_gb = int(bytes_val) // (1024 ** 3)
                    self.memory = f"{mem_gb} GB"
                break
        if not self.memory:
            self.memory = "N/A"

        # CPU
        cpu_raw = self._run("wmic cpu get Name /Value")
        for line in cpu_raw.splitlines():
            if "Name=" in line:
                self.cpu = line.split("=", 1)[-1].strip()
                break
        if not self.cpu:
            self.cpu = "N/A"

        # Disk size (C: drive)
        disk_raw = self._run("wmic logicaldisk where DeviceID='C:' get Size /Value")
        for line in disk_raw.splitlines():
            if "Size=" in line:
                size_bytes = line.split("=")[-1].strip()
                if size_bytes.isdigit():
                    size_gb = int(size_bytes) // (1024 ** 3)
                    self.disk_size = f"{size_gb} GB"
                break
        if not self.disk_size:
            self.disk_size = "N/A"
