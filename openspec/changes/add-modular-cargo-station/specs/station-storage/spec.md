## Purpose

Defines how a modular station stores cargo: capacity derived from module size, typed pools shared by columns with the same filter, how belt ports and trucks move cargo in and out of those pools, and when filters may change.

## ADDED Requirements

### Requirement: Capacity from module size
Each module column SHALL contribute one capacity unit, so module capacity is S = 1, M = 2, L = 3, XL = 4 units. The capacity unit (number of inventory slots) SHALL be a single tunable value shared by all module sizes. The number or direction of ports SHALL NOT affect capacity.

#### Scenario: XL capacity
- **WHEN** an XL module with all four filters set is attached and the capacity unit is C slots
- **THEN** the station's total cargo capacity is 4 x C slots

#### Scenario: Ports do not change capacity
- **WHEN** a player toggles ports between input and output
- **THEN** the station's capacity is unchanged

### Requirement: Typed pools shared by matching columns
The station SHALL group active columns by filter item. Each distinct item type SHALL get one pool whose size is (number of columns with that filter) x capacity unit. A pool SHALL only hold its item type.

#### Scenario: Shared pool
- **WHEN** an L module has column 1 = Copper Sheet, column 2 = Iron Plate, column 3 = Iron Plate
- **THEN** the station has a Copper Sheet pool of 1 unit and an Iron Plate pool of 2 units

#### Scenario: Pool refuses other items
- **WHEN** an Iron Plate pool has free space and Copper Sheet is offered to it
- **THEN** the Copper Sheet is not accepted into the Iron Plate pool

### Requirement: Unset columns are inactive
A column without a filter SHALL contribute no usable capacity, and its ports SHALL neither accept nor provide items.

#### Scenario: Inactive column port
- **WHEN** a belt is connected to an input port in a column with no filter
- **THEN** no items are taken from that belt

### Requirement: Input ports feed their column's pool
An input port SHALL take only items matching its column's filter from its belt and SHALL put them into that type's pool. If the pool is full or the next item on the belt does not match, the port SHALL take nothing and the belt backs up.

#### Scenario: Matching input
- **WHEN** an input port in an Iron Plate column receives Iron Plate and the Iron Plate pool has space
- **THEN** the Iron Plate is added to the Iron Plate pool

#### Scenario: Mismatched item blocks belt
- **WHEN** the front item on a belt connected to an Iron Plate input is Copper Sheet
- **THEN** the port takes nothing and the belt backs up

### Requirement: Output ports drain their column's pool
An output port SHALL provide only items of its column's filter, taken from that type's pool. Any column with the same filter SHALL be able to output from the shared pool.

#### Scenario: Output from shared pool
- **WHEN** column 2 and column 3 both filter Iron Plate and column 3 has an output port with a belt
- **THEN** that belt receives Iron Plate from the shared Iron Plate pool

### Requirement: Truck transfer respects pools
When a truck unloads, each item SHALL go only into the pool of its type while that pool has space; items without a matching pool or space SHALL stay in the truck. When a truck loads, items SHALL be taken from all pools up to the truck's free space.

#### Scenario: Partial unload
- **WHEN** a truck carrying Iron Plate and Concrete unloads at a station with only an Iron Plate pool
- **THEN** the Iron Plate moves into the pool up to its capacity and the Concrete stays in the truck

#### Scenario: Load from multiple pools
- **WHEN** a truck docks in load mode at a station with Iron Plate and Copper Sheet stock
- **THEN** both item types are loaded into the truck up to its free space

### Requirement: Filter change only when stock fits
A column filter change SHALL be allowed only if, after recomputing pools, every pool's current stock still fits within its new size. Stock fits when the number of inventory slots it needs, after merging partial stacks, is at most the pool's slot count. Otherwise the change SHALL be rejected and the UI SHALL tell the player why. Stock SHALL never be destroyed by a filter change.

#### Scenario: Change allowed when shrunk pool still fits
- **WHEN** columns 2 and 3 filter Iron Plate, the Iron Plate pool holds less than 1 unit of stock, and the player changes column 3 to Copper Sheet
- **THEN** the change is applied, the Iron Plate pool becomes 1 unit with all its stock kept, and a 1-unit Copper Sheet pool appears

#### Scenario: Change rejected when stock would not fit
- **WHEN** columns 2 and 3 filter Iron Plate, the pool holds more than 1 unit of stock, and the player changes column 3 to Copper Sheet
- **THEN** the change is rejected and nothing changes

#### Scenario: Emptying a type entirely
- **WHEN** the only column filtering Copper Sheet is changed or unset while the Copper Sheet pool is not empty
- **THEN** the change is rejected

### Requirement: Storage persists
Pool contents and layout SHALL survive save and load without losing or duplicating items.

#### Scenario: Reload keeps stock
- **WHEN** a station with stock in several pools is saved and loaded
- **THEN** each pool has the same item type, size and contents as before
