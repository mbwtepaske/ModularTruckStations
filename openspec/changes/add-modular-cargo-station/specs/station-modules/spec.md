## Purpose

Defines the sized storage modules that attach to a modular station base: their slot matrix of belt ports, per-column storage type filters with icon displays, and their placement and dismantle rules.

## ADDED Requirements

### Requirement: Module sizes
The mod SHALL provide four cargo modules with a slot matrix of columns x rows: S = 1x2, M = 2x2, L = 3x2, XL = 4x2. Each column SHALL have two slots stacked vertically.

#### Scenario: XL module layout
- **WHEN** a player builds an XL cargo module
- **THEN** it has 4 columns, each with 2 port slots and 1 display

### Requirement: Module placement on a base
A module SHALL only be placeable by snapping to the module attach point of a modular station base of the same type that has no module. A module SHALL NOT be placeable freestanding.

#### Scenario: Snap to free base
- **WHEN** a player aims a cargo module hologram at a cargo base without a module
- **THEN** the hologram snaps to the base's attach point and placement is valid

#### Scenario: Freestanding placement rejected
- **WHEN** a player aims a module hologram at ground or any other building
- **THEN** placement is invalid

### Requirement: Slot ports with input/output toggle
Every module slot SHALL hold one belt port that is either an input or an output. A newly built module SHALL start with all ports as inputs. The player SHALL be able to toggle a port between input and output from the module's interaction UI, but only while no belt is connected to that port.

#### Scenario: Toggle free port
- **WHEN** a player toggles a port that has no belt connected
- **THEN** the port switches direction and its visual changes to match

#### Scenario: Toggle connected port rejected
- **WHEN** a player tries to toggle a port that has a belt connected
- **THEN** the port keeps its direction and the UI shows that the belt must be removed first

#### Scenario: Direction persists
- **WHEN** a game with a toggled output port is saved and loaded
- **THEN** the port is still an output and belts connected to it still work

### Requirement: Column storage type filter
Each module column SHALL have a storage type filter that is either unset or one item type. A newly built module SHALL have all filters unset. The player SHALL be able to set or change a column's filter from the module's interaction UI, subject to the storage rules for filter changes.

#### Scenario: Set filter on new module
- **WHEN** a player sets column 1 of a new module to Iron Plate
- **THEN** column 1's filter is Iron Plate and its storage becomes active

### Requirement: Column filter display
Each module column SHALL have a display on top that shows the icon of the column's filter item, or an "unset" indicator when no filter is set. Displays SHALL update for all players when the filter changes.

#### Scenario: Display follows filter
- **WHEN** a column's filter changes from Iron Plate to Copper Sheet
- **THEN** the display above that column shows the Copper Sheet icon for every player

#### Scenario: Unset display
- **WHEN** a column has no filter
- **THEN** its display shows the unset indicator

### Requirement: Module dismantle keeps base
Dismantling a module SHALL leave the base in place without a module. The player SHALL receive the module's build refund and all cargo stored in the station. The base's capacity SHALL return to zero.

#### Scenario: Dismantle module with stock
- **WHEN** a player dismantles a module while the station holds cargo
- **THEN** the module is removed, the refund contains its build cost and all stored cargo, and the base remains with zero storage

#### Scenario: New module after dismantle
- **WHEN** a module was dismantled from a base
- **THEN** a new module of any size can be placed on that base

### Requirement: Module settings replicate and persist
Column filters and port directions SHALL be saved with the game and SHALL be identical for all players in multiplayer. Changes made by a client SHALL be validated by the host before they take effect.

#### Scenario: Client changes filter
- **WHEN** a client sets a column filter that the storage rules allow
- **THEN** the host applies it and every player sees the new filter and display

#### Scenario: Client request rejected
- **WHEN** a client requests a port toggle while a belt is connected to that port
- **THEN** the host rejects it and the port direction stays the same for every player
