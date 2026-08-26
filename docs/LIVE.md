# Live CSI walkthrough (Arch)

```
ESP32  --UDP 4210-->  csi-bridge.py  -->  /tmp/metafield/csi.jsonl
                                              ^
hello_view --live tails ----------------------+--> http://127.0.0.1:8765
```

## CSI source

```bash
cd ~/wifi-sensing-system/esp32
./flash.sh --standard -p /dev/ttyUSB0 -e --monitor
```

JSON `wifi_csi` on UDP **4210** (`node`, `rssi`, `csi`). Aim nodes at this host.

## Engine

```bash
cd ~/metafield-engine
git pull
chmod +x scripts/run-live.sh scripts/csi-bridge.py
./scripts/run-live.sh
```

Open http://127.0.0.1:8765 and click the canvas.

## Final test

- Badge is LIVE, not SYN
- packet count climbs while you move in front of the radios
- EST draws occupancy from that stream
- arrows / WASD move the player cube

```bash
curl -s http://127.0.0.1:8765/state | python -c 'import json,sys;s=json.load(sys.stdin);print(s["latest"]["synthetic"], s["packets"], s["live"])'
# False  <n>  True
```

`--live` never starts the synthetic generator.
