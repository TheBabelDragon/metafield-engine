# Why the running CYD is invisible

There is no mDNS, no scan, no ESP-NOW-to-PC path.

```
CYD (must be on WiFi, not LOC)
    UDP 4210  broadcast last-octet .255  only if wifiConnected
        ↓
ONE listener on the PC: csi-bridge.py  OR  dashboard.py --csi
        ↓
/tmp/metafield/csi.jsonl
        ↓
hello_view
```

Breaks that match “board is on, engine does not see it”:

1. `run-arch` used to start HUD without a UDP listener. Fixed: it starts `csi-bridge.py`.
2. CYD screen **LOC** = offline. `sendUdpCsi()` returns immediately. Join house WiFi.
3. Two listeners on 4210 (dashboard + bridge) — one fails. Run one. Dashboard now tees jsonl.
4. Guest WiFi / AP isolation drops `.255` broadcast. Same LAN, or USB serial after firmware update.
5. Auth rejects on dashboard if secret drifted. Engine jsonl path does not require auth.

See a packet:

```bash
python3 scripts/csi-bridge.py
# expect: LIVE total=N node=cyd-...
ss -ulnp | grep 4210
```
