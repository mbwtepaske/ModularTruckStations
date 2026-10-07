## Why

The vanilla truck station has a fixed 2 cargo inputs, 2 cargo outputs and 48 untyped slots, which limits throughput and forces external sorting. A modular cargo station lets players start from a compact base with the docking crane and fuel input, then attach one sized module that adds typed storage and up to 8 belt ports, giving per-type storage and port layout under player control.

Depends on `setup-sml-mod-project` (complete, archived 2026-10-06), which provided the SML starter project, the CSS engine and the empty `ModularTruckStation` mod plugin.

## What Changes

- Add a **Modular Cargo Station base** building: extends the vanilla docking station behavior (truck docking, crane load/unload cycle, station-wide load/unload mode, single fuel slot fed by one fuel belt port, power consumption that grows with module size). A base without a module stores no cargo.
- Add four **cargo modules**: S (1x2), M (2x2), L (3x2), XL (4x2) slots (columns x rows). A module snaps onto a base; a base accepts at most one module, only of the matching station type (cargo). No chaining.
- Each module slot holds a belt port that the player toggles between input and output. Toggling is only allowed while no belt is attached to that port.
- Each module column has a player-set **storage type filter** and a display on top of the column that shows the filter's item icon. A column without a filter is inactive.
- **Typed, pooled capacity**: each column contributes one tunable capacity unit. Columns with the same filter share one pool for that item type. The pool is reserved for that type only.
- Filter changes are allowed only when the affected pool's stock still fits after the change.
- Trucks unloading at the station deposit only items that have a matching pool with free space; the rest stays in the truck. Loading draws from all pools.
- Dismantling the module keeps the base and refunds the module's stored cargo; dismantling the base also dismantles the attached module.
- Unlock via a new schematic with tunable build costs.
- Fluid stations are **out of scope** (follow-up change `add-modular-fluid-station`), but the base/module/type model is defined so fluid can reuse it.

Note: this deviates from the original project idea of snapping modules onto the vanilla truck station. The base is a new building that extends the vanilla docking station class instead; vanilla truck stations are untouched. The project context in `openspec/config.yaml` and the README were updated to this concept by `setup-sml-mod-project`.

## Capabilities

### New Capabilities
- `modular-station`: the station base building: station type, docking and load/unload mode, fuel slot, power, single module attachment, dismantle cascade, save/load and replication of the base-module link.
- `station-modules`: sized modules (S/M/L/XL), placement on a base, slot ports with input/output toggle, per-column filters and icon displays, module dismantle.
- `station-storage`: capacity model (columns x capacity unit), type pools shared across columns, inactive columns, filter change rules, belt port flow per column type, truck load/unload against pools.

### Modified Capabilities
<!-- none: no existing specs -->

## Impact

- **Project/toolchain**: builds on the `ModularTruckStation` SML plugin created by `setup-sml-mod-project`; adds C++ classes and content to it.
- **New C++ classes**: station base (subclass of `AFGBuildableDockingStation`), module (subclass of `AFGBuildableFactory`), module hologram, storage pool layout logic, remote call object for client UI actions.
- **New content**: base and four module buildables, descriptors, recipes, schematic, interact widgets, port and display meshes/materials (Git LFS).
- **Vanilla dependencies (not modified)**: `AFGBuildableDockingStation`, `UFGInventoryComponent`, `UFGFactoryConnectionComponent`, `AFGDockingStationHologram`, `AFGFactoryHologram`, vanilla truck station meshes and crane animation (referenced, not changed).
- **Saves**: only new classes are introduced; existing saves without the mod's buildables are unaffected.
- **Multiplayer**: module link, filters and port directions replicate; client UI actions go through a server-validated remote call object.
