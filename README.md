# MetaField Engine

**Unreal-style cross-domain universe engine.**

Canonical World state · ECS · Fields · CSI ingest · live visual HUD.

---

## Hands-free on Arch Linux

```bash
cd metafield-engine
git pull
bash scripts/run-arch.sh
```

If you already have a `build/` folder from before `hello_view` existed:

```bash
cd metafield-engine/build
cmake ..
cmake --build . --target hello_view
./hello_view
```

The process prints the HUD address:

```
http://127.0.0.1:8765
```

That is HTTP on localhost (not HTTPS). Open it in a browser. Ctrl+C stops the view.

No extra libraries. No UDP :4210 bind.

If `/tmp/metafield/csi.jsonl` exists, the HUD is live CSI.
If not, it synthesizes an 8 Hz field so the picture still moves.
