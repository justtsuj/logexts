# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Logexts** is a WinDBG debugging extension DLL that logs Windows API calls made by a debugged process. It uses Microsoft Detours for API hooking and a binary manifest (`.lgm` files) describing API signatures to format function parameters and return values. Output goes via `OutputDebugString`.

No formal test suite exists — validation is manual via WinDBG.

## Build

Visual Studio 2019 solution (`Logexts.sln`), C++20, Platform Toolset v142.

Dependencies are installed via vcpkg:
- Detours: `D:\program\vcpkg\packages\detours_x64-windows\` and `detours_x86-windows\`
- Boost, nlohmann/json
- Windows SDK 10.0

Build the full solution:
```
msbuild Logexts.sln /p:Configuration=Release /p:Platform=x64   # 64-bit
msbuild Logexts.sln /p:Configuration=Release /p:Platform=Win32 # 32-bit
```

Output artifacts:

| Platform | Config  | Path              | File             |
|----------|---------|-------------------|------------------|
| x64      | Debug   | x64/Debug/        | logexts64.dll    |
| x64      | Release | x64/Release/      | logexts64.dll    |
| x86      | Debug   | Debug/            | logexts32.dll    |
| x86      | Release | Release/          | logexts32.dll    |

The Manifest static library also builds to the same output directories as `Manifest.lib`.

## Architecture

```
Manifest (static lib) ──┬── logexts (WinDBG extension DLL)
                        │       └── Detours for API hooking
                        │       └── manifest.lgm for API signatures
                        └── ManifestEditor (standalone EXE)
                                └── converts JSON to binary .lgm
```

### Component responsibilities

- **`logexts/`** — Core DLL. Entry points in `logexts.cpp` (`logi`, `logir`, `logd` commands). `Logger.cpp` manages hook setup. `FunctionWriter.cpp` formats call/return logs by reading parameters from stack and registers using manifest type info. `ModuleList.cpp` walks the PEB to enumerate modules without calling external APIs.
- **`Manifest/`** — Static library for reading/writing binary `.lgm` manifest files (API signatures: types, functions, categories, structs, UUIDs). File header magic: `\x25\x52\x22\x00`, header size `0x42c`.
- **`ManifestEditor/`** — Converts JSON API definitions (e.g. from Microsoft's API manifest format) into binary `.lgm` format, can merge with existing manifests.
- **`loader/`** — Test harness executable for injection scenarios.
- **`conf/`** — Static manifest files: `LogManifest.lgm` (main binary manifest), `manifest.json` / `wininet.json` (JSON source definitions).

### Hook mechanism

API hooks are implemented in assembly trampolines:
- **32-bit** (`loghook.asm`): stack-based calling convention, `LogProcessHook` called with stack arguments
- **64-bit** (`loghook64.asm`): register-based calling convention (RCX/RDX/R8/R9), shadow space reserved

Both assembly stubs call into C++ `LogProcessHook()` then invoke the original function via `LogHookCallFunction()`.

## WinDBG Usage

```
.load logexts64.dll   (or logexts32.dll for 32-bit)
logi                   inject logging into debugged process (DLL injection)
g                      continue execution
logir                  resume thread after injection point
logd                   toggle debug output mode
```

## Configuration

`Logexts.json` in the DLL directory:
```json
{
    "excludeApis": ["api_prefix_to_skip"],
    "excludeDlls": ["module.dll"],
    "isDebug": false
}
```

## Key Design Notes

- The manifest system is central — it defines all logged API signatures. Adding new APIs requires updating `conf/manifest.json` and recompiling via `ManifestEditor` into `LogManifest.lgm`.
- Module enumeration avoids any external DLL calls (uses PEB walking directly) to avoid polluting the log with its own internal calls.
- `OutputDebugString` is used for all output — view with WinDBG's `dbgout` command or a tool like DebugView.