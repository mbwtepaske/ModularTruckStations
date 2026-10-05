## 1. Toolchain install

- [x] 1.1 Link GitHub account at linker.ficsit.app, download and install the CSS Unreal Engine (latest release); verify "Unreal Engine - CSS" launches and record its install path in README
- [x] 1.2 Install/modify Visual Studio 2022 with the workloads and components from design.md (incl. MSVC v14.38, .NET 8.0 Runtime, .NET Framework 4.8.1 SDK); verify they are listed as installed in Visual Studio Installer
- [x] 1.3 Install Wwise 2023.1.14.8770 (Authoring + SDK C++, Windows VS2022) via the Wwise launcher; verify it appears under installed versions in the launcher

## 2. Starter project

- [x] 2.1 Clone `SatisfactoryModLoader` to `D:/Projects/Games/Satisfactory/SatisfactoryModLoader` and check out the revision matching game CL 502094 / SML 3.12.0 (design D2); verify the checked-out SML version in `Mods/SML/SML.uplugin` and record the commit in README
- [ ] 2.2 Integrate Wwise into `FactoryGame.uproject` (new Wwise project) and generate sound banks for all platforms; verify `GeneratedSoundBanks` exists in the Wwise project folder
- [ ] 2.3 Generate Visual Studio project files with the CSS engine and build `FactoryGame` in `Development Editor | Win64`; verify the build succeeds without errors
- [ ] 2.4 Open `FactoryGame.uproject` in the CSS editor, resolve first-launch prompts (sound bank path, audio routing), configure Alpakit dev packaging (Windows enabled, copy to `D:/Games/SatisfactoryEarlyAccess`); verify the Alpakit panel lists SML and the game path is saved

## 3. Mod plugin in repo

- [ ] 3.1 Create the `ModularTruckStation` mod with Alpakit's create-mod wizard (C++ template), set uplugin metadata (FriendlyName, Description, SML `^3.12.0`, GameVersion); verify the plugin compiles in the editor
- [ ] 3.2 Move the plugin to `Mods/ModularTruckStation/` in this repo and replace it in the starter project with a junction (`mklink /J`); verify the editor still loads the plugin and edits in the repo show up in the editor
- [ ] 3.3 Remove the blank `ModularTruckStation/` Unreal project from the repo after confirming it holds nothing beyond the generated empty module; verify `git status` no longer lists it
- [ ] 3.4 Update `.gitignore` for plugin build output (`Mods/**/Binaries`, `Intermediate`, `Saved`) and confirm `.gitattributes` LFS rules cover plugin content; verify `git status` shows only source, config and uplugin files after a build

## 4. Repo tooling and docs

- [x] 4.1 Update `ModularTruckStation.code-workspace` folders (CSS engine, starter project, game) replacing stock UE 5.6; verify the workspace opens all folders
- [x] 4.2 Update README: environment table, toolchain versions, setup steps (link, install, clone, Wwise, build, junction, Alpakit), pinned starter project commit; verify a fresh read covers every step in this task list
- [x] 4.3 Update `openspec/config.yaml` context: CSS engine path, starter project path, and the station concept (standalone base extending the vanilla docking station with one sized module; vanilla stations untouched); verify `openspec context --json` runs without errors

## 5. End-to-end verification

- [ ] 5.1 Package the empty mod with Alpakit to the game install and launch the game; verify `ModularTruckStation` is listed in the in-game SML mods menu and `FactoryGame.log` shows it loaded without errors
