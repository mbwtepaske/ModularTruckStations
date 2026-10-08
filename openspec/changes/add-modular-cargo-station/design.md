## Context

See proposal.md for motivation and specs/ for required behavior.

Current state:
- Prerequisite `setup-sml-mod-project` (complete, archived as `openspec/changes/archive/2026-10-06-setup-sml-mod-project`) provided the CSS engine, the SML starter project and an empty `ModularTruckStation` game feature plugin at `Mods/ModularTruckStation/` in this repo (junctioned into the starter project's `Mods/GameFeatures/`). The plugin has an empty C++ module and no SML root modules yet. Installed: game CL 502094, SML 3.12.0.
- FactoryGame headers are available in `CommunityResources/Headers.zip`. Relevant vanilla facts used below:
  - `AFGBuildableDockingStation` (truck station base class): one storage `mInventory` sized from `mStorageInventorySize` (default 48), one `mFuelInventory`, `mIsInLoadMode` (replicated, saved), `mIsFluidStorageInventory` (class default). `DockActor`, `Factory_CollectInput_Implementation`, `Factory_TickProducing`, `GetChildDismantleActors_Implementation` are virtual. `Factory_LoadUnloadDockedInventory` is not virtual.
  - `UFGInventoryComponent`: `Resize`, `SetAllowedItemOnIndex`, `mItemFilter` (delegate receives item class and slot index), `GetStackFromIndex`, `RemoveAllFromIndex`.
  - `UFGFactoryConnectionComponent`: `SetDirection` is callable at runtime, but `mDirection` is an `EditDefaultsOnly` default and is **not saved or replicated**. `IsConnected()` is available.
  - `AFGBuildable`: `GetDismantleInventoryReturns` and `GetDismantleRefund_Implementation` are virtual; `Factory_GrabOutput` / `Factory_PeekOutput` are native events on every buildable.
  - Holograms: `AFGDockingStationHologram`, `AFGFactoryHologram` with `TrySnapToActor`, `CheckValidPlacement`, `ConfigureActor`. `UFGConstructDisqualifier` for placement errors.
  - `UFGItemDescriptor::GetSmallIcon / GetBigIcon` give item icons. `UFGRemoteCallObject` (a `UObject`) is the player-owned channel for client-to-server RPCs; SML registers mod subclasses through the game instance module's `RemoteCallObjects` list.

## Goals / Non-Goals

**Goals:**
- One shared base/module framework where "cargo" is a type parameter, so the fluid change adds classes and logic without reworking the link, hologram, UI or persistence.
- Reuse vanilla docking, crane cycle, fuel and truck-route integration by inheritance, not reimplementation.
- All rule checks (filter change, port toggle, placement) run on the server; clients only request.

**Non-Goals:**
- Fluid stations, pipe ports, flush (follow-up change).
- Final art: placeholder meshes are acceptable; vanilla truck station mesh and crane animation are referenced as the base visual.
- Final balance numbers: capacity unit, build costs and unlock tier are tunable defaults.
- In-game config menu (SML config) for the capacity unit.

## Decisions

### D1. Code location
All classes and content go into the `ModularTruckStation` mod plugin created by `setup-sml-mod-project` (repo layout and toolchain decisions live in `openspec/changes/archive/2026-10-06-setup-sml-mod-project/design.md`). Registrations go through the plugin's SML root game world module (schematic in `mSchematics`) and root game instance module (remote call object in `RemoteCallObjects`). The plugin has neither yet; this change creates both as Blueprint assets with `bRootModule` set.

### D2. Class layout (prefix `MTS`)

```
AFGBuildableDockingStation (vanilla)
  `-- AMTSStationBase              abstract; type, module link, storage layout owner
        `-- AMTSCargoStationBase   cargo specifics (fluid sibling later)

AFGBuildableFactory (vanilla)
  `-- AMTSStationModule            abstract; columns, ports, filters, displays
        `-- AMTSCargoModule        cargo specifics; BP children S / M / L / XL

AFGFactoryHologram (vanilla)
  `-- AMTSStationModuleHologram    snap to base, validation, link on build

FMTSPoolLayout                     pure logic: columns -> pools -> slot ranges
UMTSRemoteCallObject               server RPCs for module UI actions (UFGRemoteCallObject)
EMTSStationType { Cargo, Fluid }
```

- Base extends `AFGBuildableDockingStation` so docking, crane, fuel, mode, vehicle tracking and statistics stay vanilla. The base BP has only the fuel connection, so vanilla finds no storage ports on it.
- Module extends `AFGBuildableFactory` (unpowered, like a storage container) so it gets factory tick and owns its own connection components. Each size is a BP child that defines its column count and has `columns x 2` connection components laid out in the slot matrix (rows stacked vertically).
- Alternative for the module: plain `AFGBuildable` driven entirely by the base's tick. Rejected because conveyors call `Factory_GrabOutput` on the connection's owning actor anyway, so the module must implement outputs itself; giving it factory tick for inputs keeps the logic in one place.

### D3. Storage lives in the base's vanilla inventory
The station's single cargo store is the base's existing `mInventory`, so vanilla truck load/unload, the vanilla station UI and vanilla save handling keep working on it. The base keeps at least 1 slot (class default `mStorageInventorySize = 1`) and is resized whenever the layout changes; slots outside the layout are rejected by the item filter, so a base without a module still has zero usable capacity. Spike 1.1 showed that size 0 crashes: vanilla `Factory_LoadUnloadDockedInventory` calls `AddStack`, which asserts `mInventoryStacks.Num() > 0`. With the hidden slot, docking, refuelling and the load/unload cycle work and the truck keeps its cargo. `mStorageInventorySize` must never change at runtime: the inventory saves its size as a difference from that default (`mAdjustedSizeDiff`) and is rebuilt as default + difference on load; vanilla restores stacks and size correctly.
- Alternative: a separate inventory on the module. Rejected: `Factory_LoadUnloadDockedInventory` is not virtual and always uses `mInventory`; a second inventory would require reimplementing the truck transfer.

### D4. Pool layout: deterministic slot ranges with per-slot filtering
`FMTSPoolLayout` is a pure function: (column filters, capacity unit C) -> ordered pools -> contiguous slot ranges.

```
 columns:  [Cu] [Fe] [Fe] [--]           C = 12
 pools:    Cu x1, Fe x2  (ordered by first column index)
 slots:    0..11 = Cu | 12..35 = Fe       inventory size = 36
```

- Inventory size = active columns x C, at least 1 (D3). Unset columns add no slots.
- Each slot gets `SetAllowedItemOnIndex(slot, poolItem)` (gives the UI ghost icon) and the inventory's `mItemFilter` is bound to a base function that checks the slot index against the layout, so no slot ever accepts another type.
- Applying a new layout: compute new layout, count stacks needed per item type (after merging partial stacks), reject if any pool needs more slots than it gets, otherwise move all stacks into a temporary list, resize, re-apply allowed items, and re-add stacks per pool. Never destroy items: the check runs before any mutation.
- Layout is recomputed (not loaded) after load, attach and detach, from the saved filters, so the saved inventory only needs its stacks, not its filter state.
- Alternative: one untyped pool with filters only on ports. Rejected in exploration (deadlock risk, inconsistent with fluids).

### D5. Port flow
- Inputs: module `Factory_CollectInput` loops its input connections in active columns and calls `Factory_GrabOutput` on the connected conveyor with `type = column filter`; the item is added to the base inventory (pool filter guarantees placement). Check free space for that type before grabbing so items are never lost.
- Outputs: module `Factory_PeekOutput` / `Factory_GrabOutput` (only reached when the connection has `mForwardPeekAndGrabToBuildable` set, which the module does for all slot ports) resolve the connection to its column, find the pool for the column filter and remove one item from the first non-empty slot in that pool's range.
- All inventory access goes through base methods (`AddToPool`, `TakeFromPool`, `HasSpaceInPool`) so the base owns the data and the pool range lookup.

### D6. Port direction: saved and replicated state, applied to components
The module stores `TArray<EMTSPortDirection> mPortDirections` (SaveGame, ReplicatedUsing). Because `UFGFactoryConnectionComponent::mDirection` is not saved or replicated, the module re-applies `SetDirection` from this array in `BeginPlay`, after load, and in the `OnRep` on clients (clients need the correct direction for conveyor hologram snapping). Toggle requests are rejected on the server when `IsConnected()` is true. Port visuals (input or output arrow mesh) follow the same `OnRep`.
- Default direction for every port on a new module: input.

### D7. Column filters and displays
`TArray<TSubclassOf<UFGItemDescriptor>> mColumnFilters` (SaveGame, ReplicatedUsing). A change request runs D4's apply on the server; on success the array updates. Each column has a display mesh with a dynamic material instance whose texture parameter is set from `UFGItemDescriptor::GetBigIcon(filter)` or an "unset" texture, updated in `OnRep` (and directly on the server/listen host).
- Alternative: a `UWidgetComponent` per column. Rejected: up to 4 render-target widgets per module costs more than a material parameter and adds nothing for a static icon.

### D8. Base-module link, placement and persistence
- `AMTSStationBase::mAttachedModule` and `AMTSStationModule::mStationBase`, both SaveGame + Replicated. The base exposes a module attach point component that defines where and how the module snaps.
- Module hologram: `TrySnapToActor` snaps only when the hit actor is an `AMTSStationBase` of the same `EMTSStationType` with no module; `CheckValidPlacement` adds a custom `UFGConstructDisqualifier` ("Must be attached to a free modular station base of the same type") when not snapped. `ConfigureActor` passes the base to the new module, and the module registers itself with the base on the server in `BeginPlay`.
- After load, the module re-registers with its saved base in `PostLoadGame`/`BeginPlay`; whichever side resolves last triggers the layout rebuild. Load order between the two actors is not assumed.
- Both classes carry a `SaveGame` version int so later changes (fluid, layout changes) can migrate.

### D9. Dismantle and refunds
- Base `GetChildDismantleActors` adds the attached module, so the vanilla dismantle flow removes both and shows both refunds.
- Storage refund lives with the module only: the module's `GetDismantleInventoryReturns` returns all stacks of the base storage; the base's override returns only the fuel inventory (and any vanilla extras except `mInventory`). This gives each item exactly once whether the module is dismantled alone or together with the base. A bare base has empty storage, so nothing is lost.
- Module `Dismantle_Implementation` tells the base to detach: storage is cleared (already refunded) and the inventory is resized to 0.

### D10. Client actions through a remote call object
The module UI (interact widget on the module BP) calls `UMTSRemoteCallObject` server RPCs: `Server_SetColumnFilter(module, column, item)` and `Server_TogglePort(module, slot)`. The server validates (rules from specs, plus the player is in range / the module is valid) and applies; results reach clients through replication. Rejections return a reason code that the UI shows locally. The RCO is registered in the root game instance module's `RemoteCallObjects` (D1).
- The base reuses the vanilla truck station interact widget for mode, fuel and storage view. Spike 1.3 confirmed it works with the resized inventory at 1, 12 and 48 slots: it scrolls, shows each pool's ghost icon, and manual drag and shift-click respect the item filter and land in the matching pool. Known cosmetic issue: a base without a module shows its hidden slot as one empty, locked slot.

### D11. Tunables and unlock
- Capacity unit C: `EditDefaultsOnly int32 mSlotsPerColumn` on `AMTSCargoModule`, default 12 (an XL module equals the vanilla 48 slots).
- Power: the base keeps the vanilla `mPowerConsumption` (20 MW) as its base amount and overrides `GetProducingPowerConsumptionBase()` to add `mPowerPerColumn x column count` of the attached module. `mPowerPerColumn` is `EditDefaultsOnly` on `AMTSCargoModule`, placeholder default 5 MW (XL = 40 MW). Recomputed on attach and detach.
- Build costs: recipe assets, placeholder values.
- Unlock: one mod schematic registered in the root game world module's `mSchematics` (D1), requiring the vanilla truck station unlock; contains the base and all four modules.

## Risks / Trade-offs

- [Vanilla truck unload may not respect per-slot filters, e.g. it adds items without checking the filter or drops what does not fit] -> Resolved by spike 1.2: unload adds only items the filter allows, into their pool's slots; refused items stay in the truck (log: "Add failed cause item ... is not allowed"), and load mode takes from the pools. No custom transfer needed.
- [Vanilla station code may assume a non-empty storage inventory (size 0)] -> Confirmed by spike 1.1 (assert in `AddStack`). Fallback adopted: 1 hidden slot that the item filter rejects everything for (D3).
- [Conveyor ticks may call module `Factory_GrabOutput` concurrently with the base's own factory tick] -> Keep all inventory mutation in base methods; same exposure as vanilla station outputs on their own inventory. If a race shows up, guard pool access with a critical section in the base.
- [Port direction not saved by vanilla; a belt connected to a port whose direction resets on load would break] -> D6 re-applies directions from saved state before factory tick starts; save/load test with toggled outputs is in tasks.
- [Vanilla truck station UI may show a resized/filtered inventory badly] -> Resolved by spike 1.3: the vanilla widget is usable; only the hidden slot of a bare base is visible (cosmetic).
- [Item icons as display textures could be low resolution] -> Use `GetBigIcon`.
- [Filter-change compaction rewrites the inventory; a bug could lose items] -> `FMTSPoolLayout` is pure logic with unit tests (fit check and redistribution) before it touches inventories.

## Migration Plan

New buildables only; no vanilla data is changed, so existing saves load unchanged. Removing the mod removes its buildables from a save on next load (standard SML behavior) and their stored items are lost; this is documented in the mod description. The SaveGame version fields allow later migrations.

## Open Questions

- Final capacity unit, build costs and unlock tier: balance pass after playtesting.
- Final meshes for module sizes, ports and displays: placeholder until art is made.
- Base visual: the base BP is a reparented copy of vanilla `Build_TruckStation` whose graph is a stub in the starter project, so the crane animation (`PlayDockingEffects` / `StopDockingEffects`), compass material, representation color and legacy dock area are re-implemented in the base BP. Known cosmetic issue: the crane starts in the wrong pose until its first docking; to be solved with the final base mesh.
- Which side of the base the module attach point sits on: set in the base BP during content work, does not affect logic.
