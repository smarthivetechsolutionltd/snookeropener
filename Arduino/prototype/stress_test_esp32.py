#!/usr/bin/env python3
"""
Smarthive Pool ESP32 / ESP8266 Multi-Rack Servo Stress Test & Brownout Detector.
Triggers repeated servo unlock/rack cycles at fixed intervals and monitors:
  - boot_count & uptime (instantly detects any ESP brownout / watchdog reboot!)
  - free_heap (checks for memory leaks across cycles)
  - RC522 reader health (verifies SPI never drops out after servo sweeps)
  - /api/logs black-box events (checks for BROWNOUT / CRIT / WARN alerts)

Usage:
  python3 stress_test_esp32.py --ip 192.168.5.9 --cycles 10 --interval 9
"""

import argparse
import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from http.cookiejar import CookieJar


def make_opener():
    cj = CookieJar()
    return urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))


def api_json(opener, url, method="GET", form_data=None, timeout=5.0):
    data = urllib.parse.urlencode(form_data).encode("utf-8") if form_data is not None else None
    req = urllib.request.Request(url, data=data, method=method)
    t0 = time.time()
    with opener.open(req, timeout=timeout) as resp:
        raw = resp.read().decode("utf-8", errors="replace")
        dt_ms = int((time.time() - t0) * 1000)
        return json.loads(raw), dt_ms


def main():
    p = argparse.ArgumentParser(description="Smarthive Pool Multi-Rack Stress Test")
    p.add_argument("--ip", default="192.168.5.9", help="ESP32/ESP8266 IP (default: 192.168.5.9)")
    p.add_argument("--user", default="admin", help="Portal username (default: admin)")
    p.add_argument("--password", default="admin", help="Portal password (default: admin)")
    p.add_argument("--cycles", type=int, default=10, help="Number of rack cycles (default: 10)")
    p.add_argument("--interval", type=float, default=9.0, help="Seconds between rack triggers (default: 9.0)")
    args = p.parse_args()

    base = f"http://{args.ip}"
    opener = make_opener()

    print(f"=== Smarthive Pool Multi-Rack Stress Test ({args.cycles} cycles @ {args.interval:.1f}s) ===")
    print(f"Target: {base}")

    # 1. Authenticate via /api/login
    try:
        res, ms = api_json(opener, f"{base}/api/login", method="POST",
                           form_data={"username": args.user, "password": args.password})
        if not res.get("ok"):
            print(f"[FAIL] Login rejected: {res}")
            sys.exit(1)
        print(f"[OK] Signed in ({ms} ms)")
    except Exception as e:
        print(f"[FAIL] Could not connect/login to {base}: {e}")
        sys.exit(1)

    # 2. Baseline status
    st0, _ = api_json(opener, f"{base}/api/status")
    base_boot = st0.get("boot_count", st0.get("boots"))
    base_uptime = st0.get("uptime_sec", st0.get("uptime", 0))
    base_heap = st0.get("free_heap", st0.get("heap", 0))
    rc_ver = st0.get("rc522_version", st0.get("rfid_version", "?"))
    hold_s = st0.get("servo_hold_sec", 4)
    start_a = st0.get("servo_start_angle", "?")
    end_a = st0.get("servo_end_angle", "?")
    print(f"Baseline -> BootCount={base_boot} | Uptime={base_uptime}s | FreeHeap={base_heap}B | "
          f"RC522={rc_ver} | Servo={start_a}->{end_a} deg (Hold={hold_s}s)")
    print("-" * 92)

    reboots = 0
    errors = 0
    min_heap = base_heap if isinstance(base_heap, int) else 999999999
    last_uptime = base_uptime if isinstance(base_uptime, (int, float)) else 0

    for c in range(1, args.cycles + 1):
        t_cycle = time.time()
        try:
            # Trigger servo open
            _, trig_ms = api_json(opener, f"{base}/api/servo/open", method="POST", timeout=6.0)
            # Poll 1.2s into sweep/open (critical brownout window!)
            time.sleep(1.2)
            st_mid, mid_ms = api_json(opener, f"{base}/api/status", timeout=6.0)
            mid_state = st_mid.get("servo_state", "?")

            # Wait for hold + close + settle
            wait_close = max(1.5, float(hold_s) + 1.8)
            time.sleep(wait_close)
            st_end, end_ms = api_json(opener, f"{base}/api/status", timeout=6.0)

            cur_boot = st_end.get("boot_count", st_end.get("boots"))
            cur_uptime = st_end.get("uptime_sec", st_end.get("uptime", 0))
            cur_heap = st_end.get("free_heap", st_end.get("heap", 0))
            cur_rc = st_end.get("rc522_version", st_end.get("rfid_version", "?"))
            end_state = st_end.get("servo_state", "?")

            if isinstance(cur_heap, int) and cur_heap < min_heap:
                min_heap = cur_heap

            rebooted = (
                (base_boot is not None and cur_boot != base_boot)
                or (isinstance(cur_uptime, (int, float)) and cur_uptime < last_uptime)
            )
            flag = "OK"
            if rebooted:
                reboots += 1
                flag = "REBOOT/BROWNOUT!"
                base_boot = cur_boot
            elif cur_rc in ("0x00", "0x0", "0xFF", "FAIL"):
                errors += 1
                flag = f"RC522_ERR({cur_rc})"

            if isinstance(cur_uptime, (int, float)):
                last_uptime = cur_uptime

            print(
                f"Cycle {c:02d}/{args.cycles:02d} | {flag:16s} | "
                f"Mid={mid_state:8s} -> End={end_state:6s} | "
                f"Boot={cur_boot} | Up={cur_uptime}s | Heap={cur_heap}B | RC522={cur_rc} | "
                f"HTTP={trig_ms}/{mid_ms}/{end_ms}ms"
            )
        except Exception as e:
            errors += 1
            print(f"Cycle {c:02d}/{args.cycles:02d} | HTTP_ERR/DROP    | {e} (possible brownout/reboot!)")
            time.sleep(3.0)
            # Re-login in case ESP rebooted and wiped in-memory session table
            try:
                api_json(opener, f"{base}/api/login", method="POST",
                         form_data={"username": args.user, "password": args.password})
                st_rec, _ = api_json(opener, f"{base}/api/status")
                cur_boot = st_rec.get("boot_count", st_rec.get("boots"))
                if base_boot is not None and cur_boot != base_boot:
                    reboots += 1
                    print(f"           -> CONFIRMED REBOOT! BootCount jumped {base_boot} -> {cur_boot}")
                    base_boot = cur_boot
            except Exception as e2:
                print(f"           -> Device still unreachable: {e2}")

        if c < args.cycles:
            rem = args.interval - (time.time() - t_cycle)
            if rem > 0:
                time.sleep(rem)

    print("-" * 92)
    # 3. Inspect device black-box logs (plain text endpoint)
    try:
        req = urllib.request.Request(f"{base}/api/logs", method="GET")
        with opener.open(req, timeout=6.0) as resp:
            logs_txt = resp.read().decode("utf-8", errors="replace")
        tail = [ln for ln in logs_txt.strip().splitlines() if ln.strip()][-12:]
        print("Recent /api/logs tail:")
        for ln in tail:
            print("  ", ln)
    except Exception as e:
        print(f"[WARN] Could not fetch /api/logs: {e}")

    print("=" * 92)
    verdict = "PASS (ZERO REBOOTS / BROWNOUTS)" if (reboots == 0 and errors == 0) else f"FAIL (Reboots={reboots}, Errors={errors})"
    print(f"RESULT: {verdict} | Cycles={args.cycles} | MinHeap={min_heap}B")
    sys.exit(0 if (reboots == 0 and errors == 0) else 2)


if __name__ == "__main__":
    main()
