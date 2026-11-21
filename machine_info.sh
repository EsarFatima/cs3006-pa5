#!/usr/bin/env bash
# machine_info.sh -- CS3006 Fall 2026, Programming Assignment 5 (given script)
# Run on EACH team machine, from the repository root:
#     bash machine_info.sh M2 2>&1 | tee machines/M2.txt
# The argument is that machine's label in your team (M1, M2, M3 or M4).
# Read-only: it only prints information. Do not edit the output by hand.

label="${1:-UNLABELLED}"
line() { printf '%-22s %s\n' "$1" "$2"; }

echo "==== PA5 machine report: $label ===="
line "Captured"        "$(date '+%Y-%m-%d %H:%M:%S %Z')"
line "Hostname"        "$(hostname)"
line "User"            "$(id -un)"
line "OS"              "$(. /etc/os-release 2>/dev/null; echo "${PRETTY_NAME:-unknown}")"
line "Kernel"          "$(uname -r)"
if grep -qi microsoft /proc/version 2>/dev/null; then line "Environment" "WSL2"; else line "Environment" "native Linux"; fi

echo "---- CPU ----"
LC_ALL=C lscpu | grep -E '^(Architecture|Model name|CPU\(s\)|Thread\(s\) per core|Core\(s\) per socket|Socket\(s\)|CPU max MHz|L3 cache):'
phys=$(LC_ALL=C lscpu -p=CORE,SOCKET 2>/dev/null | grep -v '^#' | sort -u | wc -l)
line "Physical cores"  "$phys"
gov=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
[ -r "$gov" ] && line "Freq. governor" "$(cat $gov)"

echo "---- Memory ----"
free -h | sed -n '1,2p'

echo "---- Power ----"
ac=$(cat /sys/class/power_supply/A{C,DP}*/online 2>/dev/null | head -1)
case "$ac" in 1) line "On AC power" "yes";; 0) line "On AC power" "NO (plug in before measuring)";;
              *) line "On AC power" "unknown (record it by hand in your write-up)";; esac

echo "---- Network ----"
ip -4 -br addr show scope global 2>/dev/null
dev=$(ip route show default 2>/dev/null | awk '{for(i=1;i<NF;i++) if($i=="dev"){print $(i+1); exit}}')
line "Default interface" "${dev:-none}"
if [ -n "$dev" ]; then
  if [ -d "/sys/class/net/$dev/wireless" ]; then
    line "Link type" "Wi-Fi"
    command -v iw >/dev/null && iw dev "$dev" link 2>/dev/null | grep -E 'SSID|freq|tx bitrate|rx bitrate'
  else
    line "Link type" "wired, or unknown (WSL2 hides it: record it by hand)"
    sp=$(cat "/sys/class/net/$dev/speed" 2>/dev/null); [ -n "$sp" ] && line "Link speed (Mb/s)" "$sp"
  fi
fi

echo "---- Toolchain ----"
line "gcc"     "$(gcc --version 2>/dev/null | head -1)"
line "mpicc"   "$(mpicc --showme:version 2>/dev/null | head -1)"
line "mpirun"  "$(mpirun --version 2>/dev/null | head -1)"
line "SHA of pa5_common.h" "$(sha256sum pa5_common.h 2>/dev/null | cut -c1-16)"
echo "==== end ===="
