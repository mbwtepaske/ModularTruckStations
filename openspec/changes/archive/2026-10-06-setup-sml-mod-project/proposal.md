## Why

The repo cannot build a Satisfactory mod yet: it contains a blank Unreal project built against the stock Epic UE 5.6.1, with no SML plugin, no FactoryGame headers and no packaging. SML mods must be built in the SML starter project with the custom CSS engine, so this toolchain has to exist before any station feature (`add-modular-cargo-station`) can be implemented.

## What Changes

- Install the modding toolchain per the SML docs: the Coffee Stain custom engine (`UnrealEngine-CSS`, 5.6.1-based), Visual Studio 2022 with the required workloads and MSVC v14.38, and Wwise 2023.1.14.8770.
- Clone and set up the SML starter project (`SatisfactoryModLoader`) outside this repo, on the branch matching the installed game (CL 502094) and SML (3.12.0); integrate Wwise, build `Development Editor | Win64`, open the editor, configure Alpakit to copy to the game install.
- Create the `ModularTruckStation` mod plugin (C++ + content, depends on SML) inside this repo and link it into the starter project's `Mods/` folder with a directory junction.
- Remove the blank `ModularTruckStation/` Unreal project from the repo.
- Update repo tooling to the new layout: `.gitignore`, VS Code workspace (CSS engine and starter project folders instead of stock UE 5.6), README environment table and setup steps, and `openspec/config.yaml` project context (engine, paths, and the station concept now being a standalone base with one module rather than modules on the vanilla station).
- Prove the pipeline end to end: an empty mod packaged with Alpakit loads in the game.

## Capabilities

### New Capabilities
<!-- none: tooling only, no game behavior. Change sets skip_specs: true -->

### Modified Capabilities
<!-- none -->

## Impact

- **Developer machine**: new engine install (stock `D:/Projects/UE_5.6` stays but is no longer used for the mod), Visual Studio components, Wwise, new starter project folder.
- **Repo**: plugin folder `Mods/ModularTruckStation/` replaces the blank project; config and docs updated. No game content or behavior yet.
- **Follow-up**: `add-modular-cargo-station` depends on this change and no longer contains setup tasks.
