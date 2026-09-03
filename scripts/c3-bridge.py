#!/usr/bin/env python3
"""Discover every USB ESP32-C3 from c3-field-swarm and unfold the fleet into jsonl."""
from __future__ import annotations

import glob
import json
import os
import re
import sys
import time
from typing import Dict, List

JSONL = os.environ.get("METAFIELD_CSI_JSONL", "/tmp/metafield/csi.jsonl")
BAUD = 115200
NODE_RE = re.compile(r"\[C3\]\s+node=(\d+)")
KV_RE = re.compile(r"\[C3\]\s+(temperature|information|energy|signal)=([0-9.+-]+)")
MEM_RE = re.compile(r"\[C3\]\s+members=(\d+)\s+coordinator=(\d+)")
TICK_RE = re.compile(r"\[C3\]\s+tick=(\d+)")


def ports() -> List[str]:
    extra = os.environ.get("METAFIELD_C3_SERIAL", "")
    found = sorted(set(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")))
    if extra:
        found = list(dict.fromkeys([x for x in extra.split(",") if x] + found))
    return [p for p in found if os.path.exists(p)]


def node_rec(node_id: int, fields: dict, *, via: str, coordinator=None,
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


class TextAcc:
    def __init__(self) -> None:
        self.node_id = 0
        self.fields: dict = {}
        self.coordinator = None
        self.tick = None

    def feed(self, line: str, via: str) -> List[dict]:
        if m := NODE_RE.search(line):
            self.node_id = int(m.group(1))
        if m := MEM_RE.search(line):
            self.coordinator = int(m.group(2))
        if m := TICK_RE.search(line):
            self.tick = int(m.group(1))
        if m := KV_RE.search(line):
            self.fields[m.group(1)] = float(m.group(2))
            if self.node_id and len(self.fields) >= 4:
                rec = node_rec(
                    self.node_id, dict(self.fields), via=via,
                    coordinator=self.coordinator, tick=self.tick,
                )
                self.fields.clear()
                return [rec]
        return []


def parse_line(raw: str, via: str, acc: TextAcc) -> List[dict]:
    raw = raw.strip()
    if raw.startswith("C3JSON "):
        try:
            pkt = json.loads(raw[7:])
        except json.JSONDecodeError:
            return []
        if isinstance(pkt, dict) and pkt.get("type") == "c3_swarm":
            return unfold(pkt, via)
    if raw.startswith("[C3]"):
        return acc.feed(raw, via)
    return []


def main() -> int:
    os.makedirs(os.path.dirname(JSONL) or ".", exist_ok=True)
    open(JSONL, "a").close()
    print(f"[c3-bridge] jsonl={JSONL}", flush=True)
    serials: Dict[str, object] = {}
    accs: Dict[str, TextAcc] = {}
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
                            accs[p] = TextAcc()
                            print(f"[c3-bridge] open {p}", flush=True)
                        except Exception as e:
                            print(f"[c3-bridge] skip {p}: {e}", flush=True)
            records: List[dict] = []
            dead = []
            for p, s in list(serials.items()):
                try:
                    raw = s.readline().decode("utf-8", errors="ignore")
                except Exception:
                    dead.append(p)
                    continue
                records.extend(parse_line(raw, p, accs[p]))
            for p in dead:
                try:
                    serials[p].close()
                except Exception:
                    pass
                serials.pop(p, None)
                accs.pop(p, None)
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
