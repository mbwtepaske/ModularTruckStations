## Purpose

Defines the modular station base: the buildable that trucks dock at, that holds the fuel slot and the station-wide load/unload mode, and that accepts exactly one matching storage module.

## ADDED Requirements

### Requirement: Typed station base
The mod SHALL provide a Modular Cargo Station base buildable whose station type is cargo. The station type of a base SHALL be fixed for the lifetime of the building.

#### Scenario: Base is built as cargo station
- **WHEN** a player builds a Modular Cargo Station base
- **THEN** the base reports station type cargo and only accepts cargo modules

### Requirement: Truck docking and load/unload cycle
The base SHALL behave as a truck docking station: trucks on a route can dock at it, and the crane runs the load or unload cycle against the station's storage, with the same cycle timing and docking behavior as the vanilla truck station.

#### Scenario: Truck docks at base
- **WHEN** a cargo truck following a route with this station arrives at the docking point
- **THEN** the truck docks and the crane load/unload cycle plays

#### Scenario: Vanilla truck stations unaffected
- **WHEN** the mod is installed
- **THEN** vanilla truck stations keep their vanilla storage, ports and behavior

### Requirement: Station-wide load/unload mode
The base SHALL have a single load/unload mode that applies to the whole station, including the attached module's storage. The mode SHALL be changeable from the base's interaction UI.

#### Scenario: Unload mode
- **WHEN** the base is in unload mode and a truck docks
- **THEN** cargo moves from the truck into the station storage

#### Scenario: Load mode
- **WHEN** the base is in load mode and a truck docks
- **THEN** cargo moves from the station storage into the truck

### Requirement: Single fuel slot
The base SHALL have exactly one fuel belt input port feeding exactly one fuel inventory slot. Docked trucks SHALL be refuelled from this slot as vanilla truck stations do.

#### Scenario: Fuel delivered by belt
- **WHEN** a belt carrying a valid vehicle fuel is connected to the base fuel port
- **THEN** fuel fills the single fuel slot and docked trucks are refuelled from it

### Requirement: Power consumption scales with module
The base SHALL require power like the vanilla truck station. Its power consumption SHALL be a base amount plus a per-column amount for each column of the attached module (S = 1, M = 2, L = 3, XL = 4 columns), whether or not the column has a filter. Without a module the base SHALL consume only the base amount. Both amounts SHALL be tunable.

#### Scenario: Bare base power
- **WHEN** a base without a module is connected to power
- **THEN** it consumes the base amount

#### Scenario: XL module power
- **WHEN** an XL module is attached to a powered base
- **THEN** the base consumes the base amount plus 4 times the per-column amount

#### Scenario: Power after module dismantle
- **WHEN** the module is dismantled from a powered base
- **THEN** the base's consumption returns to the base amount

### Requirement: Base without module stores no cargo
A base without an attached module SHALL have zero cargo storage capacity. Trucks SHALL still be able to dock and refuel.

#### Scenario: Truck unloads at bare base
- **WHEN** a truck docks at a base in unload mode that has no module
- **THEN** no cargo is transferred and the truck keeps its cargo

### Requirement: At most one module per base
A base SHALL accept at most one module, and only a module of the same station type. Modules SHALL NOT attach to other modules.

#### Scenario: Second module rejected
- **WHEN** a player tries to place a module on a base that already has a module
- **THEN** placement is invalid and the hologram shows why

#### Scenario: Wrong type rejected
- **WHEN** a player tries to place a non-cargo module on a cargo base
- **THEN** placement is invalid

### Requirement: Base dismantle cascades to module
Dismantling a base SHALL also dismantle its attached module. The player SHALL receive the build refunds of both buildings, the fuel slot contents and all stored cargo.

#### Scenario: Dismantle base with module
- **WHEN** a player dismantles a base that has a module holding stored cargo
- **THEN** both buildings are removed and the refund contains both build costs, the fuel and all stored cargo exactly once

### Requirement: Link persistence and replication
The base-module link SHALL survive save and load, and SHALL be consistent on all clients in multiplayer.

#### Scenario: Save and reload
- **WHEN** a game with a base and attached module is saved and loaded
- **THEN** the module is still attached to the same base with the same storage contents

#### Scenario: Client sees attachment
- **WHEN** a host attaches a module to a base in a multiplayer session
- **THEN** connected clients see the module as attached and see the same capacity in the station UI
