# Foundation — world-state machine

Simulation substrate first. Game engine second. Do not grow plugins until this kernel is law.

## Authority

```
observers (renderer / CSI / network)
        ↓ consume
     WorldTick
        ↓
      WORLD          ← only authority
        ↓
  simulation graph
        ↓
     substrate       Field → Chunk → Cell → Channels
        ↓
      deltas
        ↓
   journal / replay
```

`FieldDelta` is the primitive state-transition language. `WorldEvent` is the matching input language.

## Five invariants

I. **One authority.** `WorldState` is authoritative. No renderer-owned world. No CSI-owned world.

II. **No hidden mutation.** Systems observe a snapshot, emit deltas, the scheduler commits.

III. **Everything consequential is history.** event → delta → tick is representable.

IV. **Time is injectable.** No subsystem secretly calls wall-clock for epoch. Bitcoin is one `ExternalClockAnchor`, not the kernel clock.

V. **Determinism is testable.** Same state + same inputs + same clock + same rules → same `WorldTick` and same `state_hash`.

## Kernel loop

```
World world = World::create();
world.push_event(...);
WorldTick tick = world.step(dt);
assert(tick.sequence == 1);
auto hash = world.state_hash();
World restored = replay(world.history());
assert(restored.state_hash() == hash);
```

See `engine/kernel/` and `tests/world_kernel.cpp`.
