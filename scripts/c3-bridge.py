#!/usr/bin/env python3
"""USB ESP32-C3 from c3-field-swarm → jsonl bodies labeled C3."""
from __future__ import annotations

import glob
import json
import os
import re
import sys
import time
from typing import Dict, List, Optional

JSONL = os.environ.get("METAFIELD_CSI_JSONL", "/tmp/metafield/csi.jsonl")
BAUD = 115200
NODE_RE = re.compile(r"\[C3\]\s+node=(\d+)")
KV_RE = re.compile(r"\[C3\]\s+(temperature|information|energy|signal)=([0-9.+-]+)")
MEM_RE = re.compile(r"\[C3\]\s+members=(\d+)\s+coordinator=(\d+)")
TICK_RE = re.compile(r"\[C3\]\s+tick=(\d+)")
ONLINE_RE = re.compile(r"^\s*(\d{1,2})\s+ONLINE\b")
COORD_RE = re.compile(r"coordinator:\s*(\d+)")
STATE_RE = re.compile(
    r"temperature=([0-9.+-]+)\s+information=([0-9.+-]+)\s+energy=([0-9.+-]+)\s+signal=([0-9.+-]+)"
)
C3_HINT = re.compile(r"(\[C3\]|C3JSON|c3-field-swarm|coordinator:|ONLINE)")
NOT_C3 = re.compile(r"(ECHO FIELD|wifi_csi|CSIJSON|Eg7)")


def ports() -> List[str]:
    extra = os.environ.get("METAFIELD_C3_SERIAL", "")
    found = sorted(set(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")))
    if extra:
        found = list(dict.fromkeys([x for x in extra.split(",") if x] + found))
    return [p for p in found if os.path.exists(p)]


def node_rec(node_id: int, fields: dict, *, via: str, coordinator=None,
             tick=None, rssi=None, alive: bool = True, phase: str = "") -> dict:
    nid = f"c3-{int(node_id):02d}"
    temp = float(fields.get("temperature", 0) or 0)
    info = float(fields.get("information", 0) or 0)
    energy = float(fields.get("energy", 0) or 0)
    signal = float(fields.get("signal", 0) or 0)
    return {
        "type": "c3_swarm",
        "source_class": "physical",
        "synthetic": False,
        "node": nid,
        "body_id": nid,
        "body_type": "C3",
        "kind": "C3",
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
    seen = set()
    self_id = int(pkt.get("node_id") or 0)
    if self_id:
        out.append(node_rec(self_id, pkt, via=via, coordinator=pkt.get("coordinator"),
                            tick=pkt.get("tick"), phase=str(pkt.get("phase") or "")))
        seen.add(self_id)
    for nb in pkt.get("neighbors") or []:
        if not isinstance(nb, dict):
            continue
        nid = int(nb.get("node_id") or 0)
        if not nid or nid in seen:
            continue
        seen.add(nid)
        out.append(node_rec(nid, nb, via=f"{via}/gossip", coordinator=pkt.get("coordinator"),
                            tick=pkt.get("tick"), rssi=nb.get("rssi"),
                            alive=bool(nb.get("alive", True))))
    for mid in pkt.get("members") or []:
        try:
            nid = int(mid)
        except (TypeError, ValueError):
            continue
        if nid in seen:
            continue
        seen.add(nid)
        out.append(node_rec(nid, {}, via=f"{via}/member", coordinator=pkt.get("coordinator"),
                            tick=pkt.get("tick"), alive=True))
    return out


class TextAcc:
    def __init__(self) -> None:
        self.node_id = 0
        self.fields: dict = {}
        self.coordinator = None
        self.tick = None

    def feed(self, line: str, via: str) -> List[dict]:
        out: List[dict] = []
        if m := NODE_RE.search(line):
            self.node_id = int(m.group(1))
        if m := MEM_RE.search(line):
            self.coordinator = int(m.group(2))
        if m := TICK_RE.search(line):
            self.tick = int(m.group(1))
        if m := COORD_RE.search(line):
            self.coordinator = int(m.group(1))
        if m := ONLINE_RE.search(line):
            out.append(node_rec(int(m.group(1)), dict(self.fields), via=f"{via}/nodes",
                                coordinator=self.coordinator, tick=self.tick))
        if m := STATE_RE.search(line):
            self.fields = {
                "temperature": float(m.group(1)),
                "information": float(m.group(2)),
                "energy": float(m.group(3)),
                "signal": float(m.group(4)),
            }
            if self.node_id:
                out.append(node_rec(self.node_id, self.fields, via=via,
                                    coordinator=self.coordinator, tick=self.tick))
        if m := KV_RE.search(line):
            self.fields[m.group(1)] = float(m.group(2))
            if self.node_id:
                out.append(node_rec(self.node_id, dict(self.fields), via=via,
                                    coordinator=self.coordinator, tick=self.tick))
        return out


def parse_line(raw: str, via: str, acc: TextAcc) -> List[dict]:
    raw = raw.strip()
    if raw.startswith("C3JSON "):
        try:
            pkt = json.loads(raw[7:])
        except json.JSONDecodeError:
            return []
        if isinstance(pkt, dict) and pkt.get("type") == "c3_swarm":
            recs = unfold(pkt, via)
            for r in recs:
                r["body_type"] = "C3"
                r["kind"] = "C3"
            return recs
    return acc.feed(raw, via)


def open_port(serial_mod, path: str):
    s = serial_mod.Serial()
    s.port = path
    s.baudrate = BAUD
    s.timeout = 0.05
    s.dsrdtr = False
    s.rtscts = False
    s.dtr = False
    s.rts = False
    s.open()
    time.sleep(0.05)
    try:
        s.reset_input_buffer()
        s.write(b"nodes\nstatus\n")
        s.flush()
    except Exception:
        pass
    return s


def classify_line(raw: str) -> Optional[str]:
    if NOT_C3.search(raw):
        return "other"
    if C3_HINT.search(raw):
        return "c3"
    return None


def main() -> int:
    os.makedirs(os.path.dirname(JSONL) or ".", exist_ok=True)
    open(JSONL, "a").close()
    found = ports()
    print(f"[c3-bridge] jsonl={JSONL} ports={found or ['none']}", flush=True)
    serials: Dict[str, object] = {}
    accs: Dict[str, TextAcc] = {}
    kind: Dict[str, str] = {}
    skip: Dict[str, float] = {}
    seen = 0
    last_scan = 0.0
    last_poke = 0.0
    last_report = time.time()
    last_id: Dict[int, float] = {}
    raw_shown: Dict[str, int] = {}

    try:
        import serial  # type: ignore
    except ImportError:
        print("[c3-bridge] pyserial missing — sudo pacman -S --needed python-pyserial", file=sys.stderr)
        serial = None  # type: ignore

    with open(JSONL, "a", buffering=1) as out:
        while True:
            now = time.time()
            if now - last_scan >= 2.0:
                last_scan = now
                if serial is not None:
                    for p in ports():
                        if p in serials or (p in skip and now < skip[p]):
                            continue
                        try:
                            serials[p] = open_port(serial, p)
                            accs[p] = TextAcc()
                            kind[p] = "probe"
                            raw_shown[p] = 0
                            print(f"[c3-bridge] open {p} (no DTR reset)", flush=True)
                        except Exception as e:
                            print(f"[c3-bridge] skip {p}: {e}", flush=True)
                            skip[p] = now + 4.0
            if now - last_poke >= 2.0:
                last_poke = now
                for p, s in list(serials.items()):
                    if kind.get(p) == "other":
                        continue
                    try:
                        s.write(b"nodes\nstatus\n")
                    except Exception:
                        pass
            records: List[dict] = []
            dead = []
            for p, s in list(serials.items()):
                try:
                    raw = s.readline().decode("utf-8", errors="ignore")
                except Exception:
                    dead.append(p)
                    continue
                if raw.strip():
                    if raw_shown.get(p, 0) < 6:
                        raw_shown[p] = raw_shown.get(p, 0) + 1
                        print(f"[c3-bridge] {p}: {raw.strip()[:160]}", flush=True)
                    tag = classify_line(raw)
                    if tag == "other" and kind.get(p) != "c3":
                        kind[p] = "other"
                        print(f"[c3-bridge] {p} is CYD/CSI serial, not C3", flush=True)
                        continue
                    if tag == "c3":
                        kind[p] = "c3"
                    if kind.get(p) != "other":
                        recs = parse_line(raw, p, accs[p])
                        if recs:
                            kind[p] = "c3"
                        records.extend(recs)
            for p in dead:
                try:
                    serials[p].close()
                except Exception:
                    pass
                serials.pop(p, None)
                accs.pop(p, None)
                kind.pop(p, None)
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
                        f"[c3-bridge] LIVE C3 total={seen} node={rec.get('body_id')} "
                        f"fleet={sorted(last_id)} via={rec.get('via')}",
                        flush=True,
                    )
            if seen == 0 and now - last_report >= 5:
                print(
                    f"[c3-bridge] WAIT C3  ports={list(serials) or ports() or ['none']} kind={kind}",
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
