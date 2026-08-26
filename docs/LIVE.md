# Live CSI walkthrough (Arch)

```
ESP32  --UDP 4210-->  csi-bridge.py  -->  /tmp/metafield/csi.jsonl
                                              ^
hello_view --live tails ----------------------+--> HUD + terminal status
```

## Radios

```bash
cd ~/wifi-sensing-system/esp32
./flash.sh --standard -p /dev/ttyUSB0 -e --monitor
```

JSON `wifi_csi` on UDP **4210**. Aim nodes at this host.

## Engine

```bash
cd ~/metafield-engine
git pull
chmod +x scripts/run-live.sh scripts/csi-bridge.py
./scripts/run-live.sh
```

Browser opens http://127.0.0.1:8765. Click the canvas for keys.

## Final test (no curl)

Watch the **same terminal** that launched `run-live.sh`:

```
[status] WAIT  packets=0  body=-  synthetic=no
[LIVE] PASS  real CSI  body=esp32-lab  packets=1
[status] LIVE  packets=40  body=esp32-lab  synthetic=no  +
[player] keys active
```

Pass when:

- terminal prints `[LIVE] PASS`
- HUD badge is LIVE (not SYN / WAIT)
- packet count climbs as you move in front of the radios
- arrows / WASD move the player after clicking the HUD

`--live` never starts the synthetic generator. WAIT means UDP 4210 is quiet.
