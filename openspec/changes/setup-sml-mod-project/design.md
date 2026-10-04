## Context

See proposal.md for motivation.

Current state on this machine:
- Engine: `D:/Projects/UE_5.6`, stock Epic 5.6.1 (`Build.version`: `IsLicenseeVersion: 0`, branch `++UE5+Release-5.6`). SML docs require the custom CSS engine from `github.com/satisfactorymodding/UnrealEngine/releases/latest` (access by linking a GitHub account at `linker.ficsit.app`; installer `UnrealEngine-CSS-Editor-Win64.exe` + 3 `.bin` parts).
- Game: `D:/Games/SatisfactoryEarlyAccess`, build `++FactoryGame+rel-main-anniversary-2026-CL-502094`, SML 3.12.0 installed in `FactoryGame/Mods/SML`.
- Repo: blank UE project `ModularTruckStation/` (untracked; one empty runtime module), OpenSpec, VS Code workspace pointing at stock UE 5.6.

SML docs requirements (latest): Visual Studio 2022 (not 2026) with ".NET Desktop Development", "Desktop development with C++", "Game development with C++", MSVC v143 v14.38-17.8, .NET 8.0 Runtime, .NET Framework 4.8.1 SDK; Wwise 2023.1.14.8770 (Authoring + SDK C++, Windows VS2022 platform); starter project `github.com/satisfactorymodding/SatisfactoryModLoader`, branch `dev` or `master`.

## Goals / Non-Goals

**Goals:**
- A reproducible, documented setup where the mod source lives in this repo and builds/packages through the starter project.
- Verified end to end: build in editor, package with Alpakit, loaded by SML in the game.

**Non-Goals:**
- Any station gameplay classes or content (`add-modular-cargo-station`).
- Linux dedicated server cross-compile toolchain (clang): not needed for a Windows-only mod at this stage.
- CI builds.

## Decisions

### D1. Starter project lives outside the repo; mod plugin is junctioned in
```
D:/Projects/Games/Satisfactory/
  SatisfactoryModLoader/            starter project clone (not in this repo)
    FactoryGame.uproject
    Mods/
      SML/
      ModularTruckStation/  ==junction==>  ModularTruckStation repo/Mods/ModularTruckStation/
  ModularTruckStation/              this repo
    Mods/ModularTruckStation/       ModularTruckStation.uplugin, Source/, Content/, Config/
    openspec/ ...
```
- Alternative: commit the starter project into this repo. Rejected: very large, mostly third-party, changes with each SML release.
- Alternative: make this repo the mod folder itself (clone into `Mods/`). Rejected: puts openspec, workspace and docs inside the plugin folder that Alpakit packages.
- Junction (`mklink /J`) instead of symlink: works without admin or developer mode.
- Starter project path is near the drive root, not cloud-synced, per SML docs.

### D2. Starter project version pinning
Check out the starter project revision whose SML version matches the installed game (CL 502094) and SML 3.12.0: prefer a release tag if one exists for 3.12.x, else the `dev`/`master` branch the docs point to for the current game build. Record the chosen commit in README so the setup can be reproduced.

### D3. Mod plugin creation
Create the plugin with Alpakit's create-mod wizard using a C++ template, mod reference `ModularTruckStation`, then move it into the repo and junction it back (D1). The `.uplugin` declares the SML dependency with a semver range matching the installed SML (`^3.12.0`) and `GameVersion` matching the game build line SML uses. C++ class prefix for later changes: `MTS`.

### D4. Keep stock UE 5.6 untouched
The stock engine install is left in place; the workspace and docs switch to the CSS engine. Nothing in the repo references the stock engine after this change.

## Risks / Trade-offs

- [CSS engine access requires linking a GitHub account to Epic via `linker.ficsit.app`] -> Manual step for the developer, documented in README.
- [Starter project branch may not match the installed game build] -> D2 pins to a matching revision; verified by the Alpakit package actually loading in the installed game.
- [Exact MSVC toolset v14.38 may conflict with a newer default] -> Install the specific component side by side; UBT selects it per starter project config.
- [Junction in a git repo] -> The junction lives in the starter project, not in this repo, so git never sees it.

## Migration Plan

Developer-machine setup only. Rollback: remove the starter project folder and engine install; the repo keeps its plugin source.
