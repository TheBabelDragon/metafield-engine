#!/usr/bin/env python3
"""Discover every USB ESP32-C3 from c3-field-swarm and unfold the fleet into jsonl.

Opens all /dev/ttyACM* and /dev/ttyUSB* that speak [C3] or C3JSON.
One coordinator USB is enough: neighbor FieldState is in the JSON snapshot.
Multiple cables are merged and deduped by node_id.
"""
from __future__ import annotations

import glob
import json
import os
import sys
import time
from typing import Dict, List, Optional

JSONL = os.environ.get("METAFIELD_CSI_JSONL", "/tmp/metafield/csi.jsonl")
BAUD = 115200


def ports() -> List[str]:
    extra = os.environ.get("METAFIELD_C3_SERIAL", "")
    found = sorted(set(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyUSB*")))
    if extra:
        found = list(dict.fromkeys(extra.split(",") + found))
    return [p for p in found if os.path.exists(p)]


def node_rec(node_id: int, fields: dict, *, via: str, coordinator: Optional[int] = None,
             tick=None, rssi=None, alive: bool = True, phase: str = "") -> dict:
    nid = f"c3-{int(node_id):02d}"
    temp = float(fields.get("temperature", 0))
    info = float(fields.get("information", 0))
    energy = float(fields.get("energy", 0))
    signal = float(fields.get("signal", 0))
    return {
        "type": "c3_swarm",
        "source_class": "physical",
        "synthetic": False,
        "node": nid,
        "body_id": nid,
        "body_type": "c3_swarm",
        "node_id": int(node_id),
        "coordinator": coordinator,
        "tick": tick,
        "phase": phase,
        "rssi": rssi if rssi is not None else -90,
        "via": via,
        "alive": alive,
        "temperature": temp,
        "information": info,
        "energy": energy,
        "signal": signal,
        "field_regions": [
            {"region": "temperature", "observed": temp, "confidence": 1.0},
            {"region": "information", "observed": info, "confidence": 1.0},
            {"region": "energy", "observed": energy, "confidence": 1.0},
            {"region": "signal", "observed": signal, "confidence": 1.0},
        ],
        "timestamp": time.time(),
    }


def unfold(pkt: dict, via: str) -> List[dict]:
    out = []
    self_id = int(pkt.get("node_id") or 0)
    if self_id:
        out.append(node_rec(
            self_id, pkt, via=via,
            coordinator=pkt.get("coordinator"),
            tick=pkt.get("tick"),
            phase=str(pkt.get("phase") or ""),
        ))
    for nb in pkt.get("neighbors") or []:
        if not isinstance(nb, dict):
            continue
        nid = int(nb.get("node_id") or 0)
        if not nid or nid == self_id:
            continue
        out.append(node_rec(
            nid, nb, via=f"{via}/gossip",
            coordinator=pkt.get("coordinator"),
            tick=pkt.get("tick"),
            rssi=nb.get("rssi"),
            alive=bool(nb.get("alive", True)),
        ))
    return out


def parse_line(raw: str, via: str) -> List[dict]:
    raw = raw.strip()
    if raw.startswith("C3JSON "):
        try:
            pkt = json.loads(raw[7:])
        except json.JSONDecodeError:
            return []
        if isinstance(pkt, dict) and pkt.get("type") == "c3_swarm":
            return unfold(pkt, via)
    return []


def main() -> int:
    os.makedirs(os.path.dirname(JSONL) or ".", exist_ok=True)
    open(JSONL, "a").close()
    print(f"[c3-bridge] jsonl={JSONL}", flush=True)
    serials: Dict[str, object] = {}
    seen = 0
    last_scan = 0.0
    last_report = time.time()
    last_id: Dict[int, float] = {}

    try:
        import serial  # type: ignore
    except ImportError:
        print("[c3-bridge] pyserial missing — pip install pyserial", file=sys.stderr)
        serial = None  # type: ignore

    with open(JSONL, "a", buffering=1) as out:
        while True:
            now = time.time()
            if now - last_scan >= 2.0:
                last_scan = now
                if serial is not None:
                    for p in ports():
                        if p in serials:
                            continue
                        try:
                            s = serial.Serial(p, BAUD, timeout=0.05)
                            serials[p] = s
                            print(f"[c3-bridge] open {p}", flush=True)
                        except Exception as e:
                            print(f"[c3-bridge] skip {p}: {e}", flush=True)
            records: List[dict] = []
            dead = []
            for p, s in serials.items():
                try:
                    raw = s.readline().decode("utf-8", errors="ignore")
                except Exception:
                    dead.append(p)
                    continue
                records.extend(parse_line(raw, p))
            for p in dead:
                try:
                    serials[p].close()
                except Exception:
                    pass
                serials.pop(p, None)
                print(f"[c3-bridge] drop {p}", flush=True)
            for rec in records:
                nid = int(rec.get("node_id") or 0)
                prev = last_id.get(nid, 0)
                if now - prev < 0.2:
                    continue
                last_id[nid] = now
                out.write(json.dumps(rec, separators=(",", ":")) + "\n")
                out.flush()
                seen += 1
                if seen == 1 or seen % 8 == 0:
                    print(
                        f"[c3-bridge] LIVE total={seen} node={rec.get('body_id')} "
                        f"fleet={len(last_id)} via={rec.get('via')}",
                        flush=True,
                    )
            if seen == 0 and now - last_report >= 5:
                print(
                    f"[c3-bridge] WAIT  ports={list(serials) or ports() or ['none']}",
                    flush=True,
                )
                last_report = now
            if not records:
                time.sleep(0.02)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("[c3-bridge] stop", flush=True)
        sys.exit(0)
