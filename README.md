# MetaField Engine

Physical-field substrate first. Simulation is one consumer. Hardware is required.

**Kernel:** World admits Observations. A cell is an address. See `docs/FOUNDATION.md`.

**Hardware:** live ESP32 CSI (wifi-sensing-system / CYD) or optical-body-s3. `synthetic_cyd` is gone. `--synth` exits.

```bash
# 1. Flash a node
cd wifi-sensing-system/esp32 && ./flash.sh --cyd -p /dev/ttyACM0 -e --monitor

# 2. Bridge UDP 4210 -> jsonl
python3 metafield-engine/scripts/csi-bridge.py

# 3. Engine
cd metafield-engine && git pull && bash scripts/run-arch.sh
```

HUD prints `http://127.0.0.1:8765`. Badge stays WAIT until a physical packet arrives.

Optional optical body:

```bash
cd optical-body-s3 && pio run -t upload && pio device monitor
```

Optional Bitcoin tip: `export METAFIELD_BTC_HEIGHT=900001`

Kernel check (no CSI needed):

```bash
cmake -S . -B build && cmake --build build --target world_kernel_test && ./build/world_kernel_test
```
