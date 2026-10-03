# Modular Truck Station

A Satisfactory mod that adds snap-on belt port modules to the vanilla truck station, so a station can have more
cargo inputs/outputs than the default 2 in / 2 out (plus 1 fuel input).

## Environment

| What | Path |
|---|---|
| Mod repo (this folder) | `D:\Projects\Games\Satisfactory\ModularTruckStation` |
| Unreal Engine 5.6.1 | `D:\Projects\UE_5.6` |
| Satisfactory | `D:\Games\SatisfactoryEarlyAccess` |

Open `ModularTruckStation.code-workspace` in VS Code.

## Repo setup

- Git with Git LFS for Unreal and art assets (see `.gitattributes`). Run `git lfs install` once per machine.
- Spec-driven development with [OpenSpec](https://github.com/Fission-AI/OpenSpec): specs live in `openspec/`.
  Start a change with `/opsx:propose "idea"` (Claude) or `/opsx-propose "idea"` (Copilot).
