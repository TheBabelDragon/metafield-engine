# Scarcity clock

Wall-clock time is not the World epoch. Same contract as aurora-swarm-btc and metafield.

| Clock | Coordinate | Authority |
|---|---|---|
| process | `tick_count` / `simulation_time` | local experiment order |
| scarcity | `btc_height` + `btc_work` | only authoritative epoch |
| observed_at | `real_time` / packet `timestamp` | debug, liveness, never identity |

Unanchored Worlds are valid. Env/file tips are `included`, never `confirmed`.

Resolve order: `METAFIELD_BTC_HEIGHT` → `METAFIELD_CLOCK_PATH` or `/tmp/metafield/btc_clock.json` → unanchored.

`TimeState::advance` never writes height from `dt_real`.
