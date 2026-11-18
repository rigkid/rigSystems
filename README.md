# rigSystems

![preview](examples/present/img/preview.png)

ECS Update/Draw fulfillment over [rigComponent](https://github.com/rigkid/rigComponent) data.

Update: `SHierarchy`, `SCanvasUpdate`, `SModulators`, `STweens`, `SOrbitDrive`.
Draw: `SCanvasRender`, `SShapeRendering`.

Orbit mouse nav (not a system - needs a per-frame input POD): `rig::orbitNavigate` /
`orbitFrameMeshes` in `OrbitNav.h`. Bindings stay in the app.

## Example

[examples/present](examples/present/): Hierarchy orbit + Shapes.

```bash
cmake -S examples/present -B examples/present/build
cmake --build examples/present/build --target present
```

## Style

Format first-party `src/` and `examples/` with clang-format before commit (never `third_party/`).

```bash
./tools/format.sh
./tools/install-hooks.sh    # once per clone - pre-commit formats staged C/C++
```

Hooks only look for `clang-format` on PATH and warn, without formatting, when it is missing. Put it on PATH:

- **Windows:** install [LLVM](https://github.com/llvm/llvm-project/releases) (enable "Add LLVM to the system PATH"), or `winget install LLVM.LLVM`, then open a new shell. Confirm: `clang-format --version`.
- **macOS:** `brew install clang-format`
- **Linux:** `sudo apt install clang-format`

From a RigKit host checkout, `./tools/install-hooks.sh` wires every pack repo. Full policy: [RigKit packs Style](https://github.com/rigkid/RigKit/blob/main/packs/README.md#style).

[API/docs](https://rigkid.github.io/rigSystems/)
