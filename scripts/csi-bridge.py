#!/usr/bin/env python3
"""Physical CYD/ESP32 CSI → /tmp/metafield/csi.jsonl.

Listens UDP 4210 (broadcast + unicast). Announces this host on UDP 4211 so
the node can unicast back. Optional USB serial (CSIJSON lines).
"""
from __future__ import annotations

import argparse
import json
import os
import socket
import sys
import time

CSI_PORT = 4210
CMD_PORT = 4211


def local_ip() -> str:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        s.close()


def physical_record(pkt: dict, node_hint: str = "") -> dict | None:
    if not isinstance(pkt, dict):
        return None
    if pkt.get("synthetic") is True or pkt.get("source_class") == "synthetic":
        return None
    node = str(pkt.get("node") or pkt.get("body_id") or node_hint or "")
    if node.startswith("synthetic"):
        return None
    if pkt.get("type") not in (None, "wifi_csi") and "csi" not in pkt and "rssi" not in pkt:
        return None
    return {
        "type": "wifi_csi",
        "node": node or "csi-unknown",
        "body_id": node or "csi-unknown",
        "body_type": "wifi_csi",
        "rssi": pkt.get("rssi", pkt.get("rssi_dbm", -90)),
        "csi": pkt.get("csi", []),
        "timestamp": pkt.get("timestamp", time.time()),
        "source_class": "physical",
        "synthetic": False,
        "channel": pkt.get("channel"),
        "auth": pkt.get("auth"),
        "via": pkt.get("via", "udp"),
    }


def announce(sock: socket.socket, ip: str) -> None:
    msg = json.dumps({"type": "metafield_host", "cmd": "host", "ip": ip}).encode()
    try:
        sock.sendto(msg, ("255.255.255.255", CMD_PORT))
    except OSError:
        pass
    parts = ip.split(".")
    if len(parts) == 4:
        try:
            sock.sendto(msg, (".".join(parts[:3] + ["255"]), CMD_PORT))
        except OSError:
            pass


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=CSI_PORT)
    ap.add_argument("--out", default=os.environ.get("METAFIELD_CSI_JSONL", "/tmp/metafield/csi.jsonl"))
    ap.add_argument("--serial", default=os.environ.get("METAFIELD_CSI_SERIAL", ""))
    args = ap.parse_args()
    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    open(args.out, "a").close()

    ip = local_ip()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    try:
        sock.bind(("0.0.0.0", args.port))
    except OSError as e:
        print(f"[csi-bridge] FAIL bind udp:{args.port} — {e}", file=sys.stderr)
        print("[csi-bridge] 4210 is owned (dashboard.py / throne-room).", file=sys.stderr)
        print("[csi-bridge] that process must tee this jsonl, or stop it.", file=sys.stderr)
        print(f"[csi-bridge] expected file: {args.out}", file=sys.stderr)
        return 1
    sock.settimeout(0.5)

    ser = None
    if args.serial:
        try:
            import serial  # type: ignore
            ser = serial.Serial(args.serial, 115200, timeout=0.1)
            print(f"[csi-bridge] serial {args.serial}", flush=True)
        except Exception as e:
            print(f"[csi-bridge] serial open failed: {e}", file=sys.stderr)

    print(f"[csi-bridge] listen udp:{args.port} host={ip} → {args.out}", flush=True)
    print("[csi-bridge] announcing metafield_host on udp:4211", flush=True)

    n = 0
    last_report = time.time()
    last_announce = 0.0
    with open(args.out, "a", buffering=1) as out:
        while True:
            now = time.time()
            if now - last_announce >= 2.0:
                announce(sock, ip)
                last_announce = now
            try:
                data, addr = sock.recvfrom(65535)
            except socket.timeout:
                data, addr = b"", ("", 0)
            except KeyboardInterrupt:
                break

            pkts = []
            if data:
                try:
                    pkt = json.loads(data.decode("utf-8", errors="ignore"))
                    rec = physical_record(pkt)
                    if rec:
                        rec["from"] = addr[0]
                        pkts.append(rec)
                except Exception:
                    pass

            if ser is not None:
                try:
                    raw = ser.readline().decode("utf-8", errors="ignore").strip()
                except Exception:
                    raw = ""
                if raw.startswith("CSIJSON "):
                    try:
                        rec = physical_record(json.loads(raw[8:]))
                        if rec:
                            rec["via"] = "serial"
                            pkts.append(rec)
                    except Exception:
                        pass

            for rec in pkts:
                out.write(json.dumps(rec, separators=(",", ":")) + "\n")
                out.flush()
                n += 1
                if n == 1 or n % 10 == 0 or now - last_report >= 2:
                    print(
                        f"[csi-bridge] LIVE total={n} node={rec.get('body_id')} via={rec.get('via')}",
                        flush=True,
                    )
                    last_report = now
            if n == 0 and now - last_report >= 5:
                print(f"[csi-bridge] WAIT  no CYD yet  host={ip}  file={args.out}", flush=True)
                last_report = now
    print(f"[csi-bridge] stop packets={n}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
