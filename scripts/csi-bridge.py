#!/usr/bin/env python3
"""UDP 4210 wifi_csi → /tmp/metafield/csi.jsonl for hello_view."""
import argparse, json, os, socket, sys, time

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=4210)
    ap.add_argument("--out", default=os.environ.get("METAFIELD_CSI_JSONL", "/tmp/metafield/csi.jsonl"))
    args = ap.parse_args()
    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        sock.bind(("0.0.0.0", args.port))
    except OSError as e:
        print(f"[csi-bridge] FAIL bind udp:{args.port} — {e}", file=sys.stderr)
        print("[csi-bridge] another process (dashboard.py) probably owns 4210", file=sys.stderr)
        return 1
    sock.settimeout(1.0)
    print(f"[csi-bridge] listen udp:{args.port} → {args.out}", flush=True)
    n = 0
    last_report = time.time()
    with open(args.out, "a", buffering=1) as out:
        while True:
            try:
                data, addr = sock.recvfrom(65535)
            except socket.timeout:
                now = time.time()
                if n > 0 and now - last_report >= 2:
                    print(f"[csi-bridge] waiting  total={n}", flush=True)
                    last_report = now
                continue
            except KeyboardInterrupt:
                break
            try:
                pkt = json.loads(data.decode("utf-8", errors="ignore"))
            except Exception:
                continue
            if not isinstance(pkt, dict):
                continue
            if pkt.get("type") not in (None, "wifi_csi"):
                if "csi" not in pkt and "rssi" not in pkt:
                    continue
            node = pkt.get("node") or pkt.get("body_id") or addr[0]
            rec = {
                "type": "wifi_csi",
                "node": str(node),
                "body_id": str(node),
                "body_type": "wifi_csi",
                "rssi": pkt.get("rssi", pkt.get("rssi_dbm", -90)),
                "csi": pkt.get("csi", []),
                "timestamp": pkt.get("timestamp", time.time()),
                "synthetic": False,
            }
            out.write(json.dumps(rec, separators=(",", ":")) + "\n")
            out.flush()
            n += 1
            now = time.time()
            if n == 1 or n % 10 == 0 or now - last_report >= 2:
                print(f"[csi-bridge] LIVE total={n} from {addr[0]} node={node}", flush=True)
                last_report = now
    print(f"[csi-bridge] stop packets={n}", flush=True)
    return 0

if __name__ == "__main__":
    sys.exit(main())
