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
| Starter project | `master` commit `d2162c999b` ("Update headers to CL502094"), local branch `mts-cl502094` |
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
   git checkout -b mts-cl502094 d2162c999b
   ```
   Check `Mods\SML\SML.uplugin` shows `"SemVersion": "3.12.0"`. The `v3.12.0` tag is not enough: its FactoryGame
   headers predate game CL 502094, and a mod that subclasses game classes then fails to load with "Entry Point Not
   Found" (e.g. `AFGBuildableFactory::OnBuildEffectActorFinished`). The starter project headers must match the
   installed game build.
6. **Wwise integration.** In the Wwise Launcher, Unreal Engine tab, use "Integrate Wwise in Project..." on
   `SatisfactoryModLoader\FactoryGame.uproject` (new Wwise project, version 2023.1.14.8770). Open the created Wwise
   project in Wwise Authoring, Project Settings > SoundBanks, and enable the metadata the Unreal integration needs:
   "Generate JSON Metadata", "Generate Per Bank Metadata", "Object GUID", "Object Path", "Max Attenuation" and
   "Estimated Duration". Then generate sound banks for all platforms (SoundBank Manager > Generate All, or with
   Authoring closed: `WwiseConsole.exe generate-soundbank <path to .wproj>`). `GeneratedSoundBanks` must contain
   `ProjectInfo.json`, and the editor log must show no `LogWwiseProjectDatabase` errors.
7. **Build.** Right-click `FactoryGame.uproject` > "Generate Visual Studio project files", open `FactoryGame.sln` in
   Visual Studio 2022 and build `FactoryGame` in `Development Editor | Win64`. Command line equivalent (the editor
   target is `FactoryEditor`; the installed engine has no `GenerateProjectFiles.bat`):
   ```
   set E=D:\Projects\Games\Satisfactory\UnrealEngine-CSS\Engine
   set P=D:\Projects\Games\Satisfactory\SatisfactoryModLoader\FactoryGame.uproject
   "%E%\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="%P%" -game -rocket -progress
   "%E%\Build\BatchFiles\Build.bat" FactoryEditor Win64 Development -Project="%P%" -WaitMutex
   ```
8. **Editor and Alpakit.** Open `FactoryGame.uproject` (Unreal Engine - CSS) and accept the first-launch prompts
   (sound bank path, audio routing). In Alpakit (toolbar, "Alpakit Dev") enable Windows, enable "Copy to Game Path"
   with `D:\Games\SatisfactoryEarlyAccess`, and disable Windows Server and Linux Server (Linux Server needs the clang
   cross-compile toolchain, which this setup does not install). The mod itself already exists in this repo; on a fresh
   machine skip Alpakit's "Create Mod" and do step 9.
9. **Mod plugin junction.** With the editor closed, link the plugin from this repo into the starter project. The mod is a
   game feature plugin, so it must sit under `Mods\GameFeatures\`:
   ```
   mklink /J D:\Projects\Games\Satisfactory\SatisfactoryModLoader\Mods\GameFeatures\ModularTruckStation D:\Projects\Games\Satisfactory\ModularTruckStation\Mods\ModularTruckStation
   ```
   (`cmd`; a junction needs no admin rights.) Regenerate project files and build again (step 7).
10. **Package and test.** In Alpakit Dev, package `ModularTruckStation`, start the game and check the mod is listed in
    the SML mods menu.

## Repo setup

- Git with Git LFS for Unreal and art assets (see `.gitattributes`). Run `git lfs install` once per machine.
- Spec-driven development with [OpenSpec](https://github.com/Fission-AI/OpenSpec): specs live in `openspec/`.
  Start a change with `/opsx:propose "idea"` (Claude) or `/opsx-propose "idea"` (Copilot).
