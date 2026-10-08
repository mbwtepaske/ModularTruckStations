## 1. Vanilla behavior spikes

- [x] 1.1 Spike: minimal `AMTSStationBase` subclass of `AFGBuildableDockingStation` with `mStorageInventorySize = 0`, built in game with a truck route; verify a truck docks, refuels and completes the load/unload cycle without errors in the log
- [x] 1.2 Spike: resize the base inventory at runtime and apply per-slot `SetAllowedItemOnIndex` + `mItemFilter`; unload a truck with mixed cargo; verify only matching items move and the rest stays in the truck (if not, implement the D3/Risks fallback transfer in `Factory_TickProducing` and re-verify)
- [x] 1.3 Spike: open the base with the vanilla truck station interact widget at sizes 0, 12 and 48; record whether it is usable, and decide reuse vs. mod widget in design.md

## 2. Pool layout logic

- [x] 2.1 Implement `FMTSPoolLayout` (column filters + capacity unit -> ordered pools -> slot ranges, inventory size) and verify with editor automation tests: S/M/L/XL sizes, shared pools, unset columns, pool ordering
- [x] 2.2 Implement the fit check and redistribution plan (stacks per type after merge vs. new pool slots) and verify automation tests for: allowed shrink, rejected shrink, rejected removal of a non-empty type, no item count change across redistribution

## 3. Station base

- [x] 3.1 Implement `EMTSStationType`, `AMTSStationBase` and `AMTSCargoStationBase`: type, module attach point, `mAttachedModule` (SaveGame, Replicated), SaveGame version field; verify it compiles and the base still builds, docks and refuels in game
- [x] 3.2 Implement base storage methods (`ApplyLayout`, `AddToPool`, `TakeFromPool`, `HasSpaceInPool`, item filter binding) using `FMTSPoolLayout`; verify an automation or in-game test applies a layout to a real `UFGInventoryComponent` without losing items
- [x] 3.3 Implement attach/detach (resize to layout or to 0) and base `GetDismantleInventoryReturns` (fuel only) + `GetChildDismantleActors` (module); verify dismantling a bare base refunds build cost and fuel
- [x] 3.4 Create the base BP (vanilla truck station mesh and crane animation referenced, fuel connection only, `AFGDockingStationHologram`, descriptor, recipe); verify it can be built in game and a truck route docks at it

## 4. Station module

- [x] 4.1 Implement `AMTSStationModule` / `AMTSCargoModule`: column count, `mColumnFilters`, `mPortDirections` (SaveGame, ReplicatedUsing), `mSlotsPerColumn` (default 12), base link, SaveGame version; verify it compiles
- [x] 4.2 Map connection components to (column, row) and apply port directions in `BeginPlay`, after load and in `OnRep`; verify a toggled output port still feeds its belt after save/load
- [x] 4.3 Implement input flow in `Factory_CollectInput` (grab only the column type, only when the pool has space) and output flow in `Factory_PeekOutput` / `Factory_GrabOutput` from the column's pool; verify in game: matching input fills pool, mismatched item backs up the belt, an input port in an unset column takes nothing, two columns with same filter share one pool for output
- [x] 4.4 Implement module dismantle: storage refund via `GetDismantleInventoryReturns`, detach in `Dismantle_Implementation`; verify module-only dismantle refunds module cost + all stock and the base remains with zero storage, and base dismantle with module refunds everything exactly once
- [ ] 4.5 Implement column displays (dynamic material with `GetBigIcon` or unset texture, updated on server and in `OnRep`); verify the icon updates when a filter changes
- [x] 4.6 Implement power scaling (base override of `GetProducingPowerConsumptionBase`, `mPowerPerColumn` on the module); verify in game: bare base 20 MW, XL module 40 MW, back to 20 MW after module dismantle

## 5. Placement

- [x] 5.1 Implement `AMTSStationModuleHologram` (snap to free base of same type, custom construct disqualifier otherwise, pass base in `ConfigureActor`); verify in game: snaps to free cargo base, rejected on ground, rejected on a base that already has a module; verify the station type check with an automation test of the snap predicate (no non-cargo module exists in this change)
- [ ] 5.2 Create S, M, L and XL module BPs (placeholder meshes, `columns x 2` connection components in a vertical 2-row matrix, display meshes, descriptors, recipes); verify each builds on a base and shows the expected number of ports and displays

## 6. UI and multiplayer actions

- [ ] 6.1 Implement `UMTSRemoteCallObject` (subclass of `UFGRemoteCallObject`) with `Server_SetColumnFilter` and `Server_TogglePort` (server validation, reason codes); create the root game instance module BP (`bRootModule`) and add the RCO to its `RemoteCallObjects`; verify in single player that a filter change and a free-port toggle go through the RCO and apply
- [ ] 6.2 Create the module interact widget (per-column filter picker, per-slot direction toggle, rejection messages); verify the toggle is refused with a message while a belt is connected and the filter change is refused when stock would not fit
- [x] 6.3 Hook up the base UI per the decision in 1.3 (vanilla widget or mod widget with mode toggle, fuel slot, storage view); verify mode changes affect truck transfer direction

## 7. Unlock and integration

- [ ] 7.1 Add the mod schematic (requires vanilla truck station unlock) with the base and four modules; create the root game world module BP (`bRootModule`) and add the schematic to its `mSchematics`; verify the schematic appears and unlocks all five buildables in a new save
- [ ] 7.2 End-to-end test in single player: build base + XL module, set mixed filters, belts in and out, truck route in both modes, save, reload, change filters, dismantle module, re-attach M module, dismantle base; verify every scenario in specs/ behaves as specified and no items are lost or duplicated
- [ ] 7.3 Load a save made before the mod was installed and a save with vanilla truck stations; verify vanilla stations behave unchanged
- [ ] 7.4 Multiplayer verification in the packaged game (listen server host + client; dedicated server if available): base-module link, capacity, filters, displays and port directions replicate (3.1, 4.1, 4.5); a client can change a filter and toggle a free port, and is refused for a connected port (6.1); mode changes from the client affect truck transfer (6.3); rerun the 7.2 scenario with a client
