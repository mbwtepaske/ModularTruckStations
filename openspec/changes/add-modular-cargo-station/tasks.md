## 1. Vanilla behavior spikes

- [ ] 1.1 Spike: minimal `AMTSStationBase` subclass of `AFGBuildableDockingStation` with `mStorageInventorySize = 0`, built in game with a truck route; verify a truck docks, refuels and completes the load/unload cycle without errors in the log
- [ ] 1.2 Spike: resize the base inventory at runtime and apply per-slot `SetAllowedItemOnIndex` + `mItemFilter`; unload a truck with mixed cargo; verify only matching items move and the rest stays in the truck (if not, implement the D3/Risks fallback transfer in `Factory_TickProducing` and re-verify)
- [ ] 1.3 Spike: open the base with the vanilla truck station interact widget at sizes 0, 12 and 48; record whether it is usable, and decide reuse vs. mod widget in design.md

## 2. Pool layout logic

- [ ] 2.1 Implement `FMTSPoolLayout` (column filters + capacity unit -> ordered pools -> slot ranges, inventory size) and verify with editor automation tests: S/M/L/XL sizes, shared pools, unset columns, pool ordering
- [ ] 2.2 Implement the fit check and redistribution plan (stacks per type after merge vs. new pool slots) and verify automation tests for: allowed shrink, rejected shrink, rejected removal of a non-empty type, no item count change across redistribution

## 3. Station base

- [ ] 3.1 Implement `EMTSStationType`, `AMTSStationBase` and `AMTSCargoStationBase`: type, module attach point, `mAttachedModule` (SaveGame, Replicated), SaveGame version field; verify it compiles and properties replicate in a listen-server PIE session
- [ ] 3.2 Implement base storage methods (`ApplyLayout`, `AddToPool`, `TakeFromPool`, `HasSpaceInPool`, item filter binding) using `FMTSPoolLayout`; verify an automation or in-game test applies a layout to a real `UFGInventoryComponent` without losing items
- [ ] 3.3 Implement attach/detach (resize to layout or to 0) and base `GetDismantleInventoryReturns` (fuel only) + `GetChildDismantleActors` (module); verify dismantling a bare base refunds build cost and fuel
- [ ] 3.4 Create the base BP (vanilla truck station mesh and crane animation referenced, fuel connection only, `AFGDockingStationHologram`, descriptor, recipe); verify it can be built in game and a truck route docks at it

## 4. Station module

- [ ] 4.1 Implement `AMTSStationModule` / `AMTSCargoModule`: column count, `mColumnFilters`, `mPortDirections` (SaveGame, ReplicatedUsing), `mSlotsPerColumn` (default 12), base link, SaveGame version; verify compile and replication in listen-server PIE
- [ ] 4.2 Map connection components to (column, row) and apply port directions in `BeginPlay`, after load and in `OnRep`; verify a toggled output port still feeds its belt after save/load
- [ ] 4.3 Implement input flow in `Factory_CollectInput` (grab only the column type, only when the pool has space) and output flow in `Factory_PeekOutput` / `Factory_GrabOutput` from the column's pool; verify in game: matching input fills pool, mismatched item backs up the belt, two columns with same filter share one pool for output
- [ ] 4.4 Implement module dismantle: storage refund via `GetDismantleInventoryReturns`, detach in `Dismantle_Implementation`; verify module-only dismantle refunds module cost + all stock and the base remains with zero storage, and base dismantle with module refunds everything exactly once
- [ ] 4.5 Implement column displays (dynamic material with `GetBigIcon` or unset texture, updated on server and in `OnRep`); verify the icon updates for host and client when a filter changes

## 5. Placement

- [ ] 5.1 Implement `AMTSStationModuleHologram` (snap to free base of same type, custom construct disqualifier otherwise, pass base in `ConfigureActor`); verify in game: snaps to free cargo base, rejected on ground, rejected on a base that already has a module
- [ ] 5.2 Create S, M, L and XL module BPs (placeholder meshes, `columns x 2` connection components in a vertical 2-row matrix, display meshes, descriptors, recipes); verify each builds on a base and shows the expected number of ports and displays

## 6. UI and multiplayer actions

- [ ] 6.1 Implement `AMTSRemoteCallObject` with `Server_SetColumnFilter` and `Server_TogglePort` (server validation, reason codes) and register it via the mod's SML module; verify a client in a listen-server session can change a filter and toggle a free port
- [ ] 6.2 Create the module interact widget (per-column filter picker, per-slot direction toggle, rejection messages); verify the toggle is refused with a message while a belt is connected and the filter change is refused when stock would not fit
- [ ] 6.3 Hook up the base UI per the decision in 1.3 (vanilla widget or mod widget with mode toggle, fuel slot, storage view); verify mode changes from host and client affect truck transfer direction

## 7. Unlock and integration

- [ ] 7.1 Add the mod schematic (requires vanilla truck station unlock) with the base and four modules, registered via SML game world module; verify the schematic appears and unlocks all five buildables in a new save
- [ ] 7.2 End-to-end test in a dedicated or listen-server session: build base + XL module, set mixed filters, belts in and out, truck route in both modes, save, reload, change filters, dismantle module, re-attach M module, dismantle base; verify every scenario in specs/ behaves as specified and no items are lost or duplicated
- [ ] 7.3 Load a save made before the mod was installed and a save with vanilla truck stations; verify vanilla stations behave unchanged
