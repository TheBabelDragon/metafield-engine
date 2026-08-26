# Live Field Loop v0.1

CSI → bind frozen sample → CsiInject deltas → diffusion/decay/advection → one FieldTick → HUD / live.ndjson

```bash
cd ~/metafield-engine && git pull
cd build && cmake ..
cmake --build . --target live_loop hello_view
./live_loop
../scripts/run-live.sh
./hello_view --replay /tmp/metafield/live.ndjson
```

Evaluate never writes Field. Replay only applies recorded ticks.
