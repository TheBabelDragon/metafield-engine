# plugins/csi

Adapter around the existing CSI snake. The engine does **not** bind UDP :4210.

```
CYD CSI → ESP-NOW → bridge → UDP :4210 → throne-room metafield_bridge
                                              │
                                    /tmp/metafield/csi.jsonl
                                              │
                                    metafield-engine CsiField
```

Override path with `METAFIELD_CSI_JSONL`.
If the file is missing, `hello_csi` generates synthetic packets so the binary is always runnable.
