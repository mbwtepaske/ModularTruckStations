# Modular Truck Station

A Satisfactory mod that adds a standalone truck station base. The base extends the vanilla truck docking station and takes
one sized module with extra belt input/output ports, so a station can have more cargo inputs/outputs than the vanilla
2 in / 2 out (plus 1 fuel input). Vanilla stations are untouched.

## Environment

| What | Path |
|---|---|
| Mod repo (this folder) | `D:\Projects\Games\Satisfactory\ModularTruckStation` |
| Mod plugin source | `Mods\ModularTruckStation` (in this repo) |
| Unreal Engine - CSS 5.6.1 (`5.6.1-CSS`) | `D:\Projects\Games\Satisfactory\UnrealEngine-CSS` |
| Starter project (SatisfactoryModLoader) | `D:\Projects\Games\Satisfactory\SatisfactoryModLoader` |
| Wwise 2023.1.14.8770 | `D:\Audiokinetic\Wwise_2023.1.14.8770` |
| Satisfactory | `D:\Games\SatisfactoryEarlyAccess` |

Open `ModularTruckStation.code-workspace` in VS Code.

### Toolchain versions

| Tool | Version |
|---|---|
| Game | `++FactoryGame+rel-main-anniversary-2026-CL-502094` |
| SML | 3.12.0 |
| Starter project | tag `v3.12.0`, commit `1a7d2ca3a4281cf589bd842a814fd7a55eac4a99`, local branch `mts-v3.12.0` |
| Engine | Unreal Engine - CSS `5.6.1-83++5.6.1-CSS` |
| Visual Studio | 2022 (not 2026): ".NET desktop development", "Desktop development with C++", "Game development with C++", MSVC v143 v14.38-17.8, .NET 8.0 Runtime, .NET Framework 4.8.1 SDK |
| Wwise | 2023.1.14.8770: Authoring, SDK (C++), Windows Visual Studio 2022 platform |

## Setup on a new machine

Follows the [SML modding docs](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/dependencies.html).
Keep the engine and the starter project near the drive root and out of cloud-synced folders.

1. **Engine access.** Link your GitHub account to your Epic account at <https://linker.ficsit.app>, then accept the
   invite to the `satisfactorymodding` GitHub organization.
2. **CSS engine.** Download `UnrealEngine-CSS-Editor-Win64.exe` and its 3 `.bin` parts from
   <https://github.com/satisfactorymodding/UnrealEngine/releases/latest> into one folder and run the installer with
   install folder `D:\Projects\Games\Satisfactory\UnrealEngine-CSS`. The result must be
   `UnrealEngine-CSS\Engine\...` and `UnrealEngine-CSS\SetupScripts\`. If the engine is moved, run
   `SetupScripts\Register.bat` again so the `5.6.1-CSS` build id points at the new folder.
3. **Visual Studio 2022.** Install with the workloads and components in the table above.
4. **Wwise.** In the Wwise Launcher install 2023.1.14.8770 with the components in the table above. Make sure the
   `WWISEROOT` user variable points at that version.
5. **Starter project.**
   ```
   cd D:\Projects\Games\Satisfactory
   git clone https://github.com/satisfactorymodding/SatisfactoryModLoader.git
   cd SatisfactoryModLoader
   git checkout -b mts-v3.12.0 v3.12.0
   ```
   Check `Mods\SML\SML.uplugin` shows `"SemVersion": "3.12.0"`.
6. **Wwise integration.** In the Wwise Launcher, Unreal Engine tab, use "Integrate Wwise in Project..." on
   `SatisfactoryModLoader\FactoryGame.uproject` (new Wwise project, version 2023.1.14.8770). Open the created Wwise
   project in Wwise Authoring and generate sound banks for all platforms; `GeneratedSoundBanks` must exist in the Wwise
   project folder.
7. **Build.** Right-click `FactoryGame.uproject` > "Generate Visual Studio project files", open `FactoryGame.sln` in
   Visual Studio 2022 and build `FactoryGame` in `Development Editor | Win64`.
8. **Editor and Alpakit.** Open `FactoryGame.uproject` (Unreal Engine - CSS) and accept the first-launch prompts
   (sound bank path, audio routing). In Alpakit (toolbar, "Alpakit Dev") enable Windows, enable copying to the game and
   set the game path to `D:\Games\SatisfactoryEarlyAccess`.
9. **Mod plugin junction.** With the editor closed, link the plugin from this repo into the starter project:
   ```
   mklink /J D:\Projects\Games\Satisfactory\SatisfactoryModLoader\Mods\ModularTruckStation D:\Projects\Games\Satisfactory\ModularTruckStation\Mods\ModularTruckStation
   ```
   (`cmd`; a junction needs no admin rights.) Regenerate project files and build again.
10. **Package and test.** In Alpakit Dev, package `ModularTruckStation`, start the game and check the mod is listed in
    the SML mods menu.

## Repo setup

- Git with Git LFS for Unreal and art assets (see `.gitattributes`). Run `git lfs install` once per machine.
- Spec-driven development with [OpenSpec](https://github.com/Fission-AI/OpenSpec): specs live in `openspec/`.
  Start a change with `/opsx:propose "idea"` (Claude) or `/opsx-propose "idea"` (Copilot).
