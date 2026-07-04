# orc-plugin_skeleton

External Decode-Orc stage plugin skeleton repository.

This repository is intended to be the canonical starter template for third-party stage plugins.

## What It Contains

- Minimal stage implementation: `skeleton_passthrough`
- Required plugin entrypoints:
	- `orc_get_stage_plugin_descriptor`
	- `orc_register_stage_plugin`
- Local smoke tests for stage metadata and entrypoint registration
- Cross-platform CI workflow targets for Linux, macOS, and Windows
- Release workflow that uploads platform plugin artifacts

## SDK Contract (Decode-Orc 2.x — host ABI 5 / plugin API 2)

- Include only the public SDK umbrella header in plugin/stage code:

```cpp
#include <orc/plugin/orc_plugin_sdk.h>
```

- Export the two required entrypoints:
	- `orc_get_stage_plugin_descriptor`
	- `orc_register_stage_plugin`

- Build the descriptor with the `ORC_STAGE_PLUGIN_DESCRIPTOR` macro, which
	fills in the host ABI version, plugin API version, and toolchain tag from
	the SDK the plugin is compiled against. The host requires all three to
	match exactly at load time; plugins built against a 1.x SDK are refused.

## Plugin Version Source

- The plugin descriptor version is set at CMake configure time.
- Default value is `project(VERSION ...)` from `CMakeLists.txt`.
- In GitHub Actions tag builds, tags like `v0.2.5` are automatically mapped to plugin version `0.2.5`.
- You can always override manually with:

```bash
cmake -S . -B build -DORC_PLUGIN_VERSION=1.2.3
```

## Preview Hook

This skeleton stage shows how external transform plugins integrate with Decode-Orc's preview dialog under the Decode-Orc 2.x structured preview contract, which replaced the legacy `PreviewableStage` interface. Stages no longer render preview images themselves — they declare what data they can supply and the host does all rendering.

In `SkeletonPassthroughStage`, the preview integration is exposed through the `IStagePreviewCapability` interface:

- `get_preview_capability()`
  - Returns a `StagePreviewCapability` declaring the data types the stage can supply (composite or Y/C, PAL or NTSC), the navigable frame range, and the active picture geometry / display aspect ratio.
  - Returns an `is_valid() == false` capability until data has been loaded.

Execution and preview are connected by a small cache:

- `execute(...)` passes input artifact `0` through unchanged
- The same input is cached as a `VideoFrameRepresentation` (frame-based CVBS_U10_4FSC data, the Decode-Orc 2.x signal contract)
- `get_preview_capability()` describes that cached representation to the host, which renders the preview itself

Plugins built against the installed SDK package can simply return `PreviewHelpers::make_signal_preview_capability(cached_output_)`; this skeleton composes the capability by hand so it also builds in the header-only in-tree CI configuration, which links no host libraries. Stages that modify sample data should extend `VideoFrameRepresentationWrapper` instead of forwarding the input artifact unchanged — see the "Transform stages" section of the decode-orc plugin SDK guide (`docs/technical/plugin-sdk.md` in the decode-orc repository).

## Local Build (installed SDK)

1. Install decode-orc (or at minimum its plugin SDK package) to a prefix.
2. Configure and build this plugin against that install prefix:

```bash
cmake -S . -B build \
	-DCMAKE_PREFIX_PATH=/absolute/path/to/decode-orc-install
cmake --build build --parallel
```

3. Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Local Build (against in-tree decode-orc SDK)

1. Clone decode-orc and this repository as sibling directories.
2. Enter the Nix development shell (recommended):

```bash
nix develop
```

This repository's flake tracks the same nixpkgs release as decode-orc's flake, so the shell provides the same C++ toolchain as a decode-orc host built from its own `nix develop` / `nix build`. This matters: since Decode-Orc 2.0 (host ABI 5) the loader requires the plugin's toolchain tag (compiler family + major version + standard library, e.g. `gcc15/libstdc++`) to equal the host's exactly, and rejects the plugin otherwise. If you build the host from a different environment, build the plugin from that same environment.

3. Configure and build:

```bash
cmake -S . -B build \
	-DORC_INTREE_SDK_DIR=/absolute/path/to/decode-orc \
	-DBUILD_TESTS=ON
cmake --build build --parallel
```

4. Run tests:

```bash
ctest --test-dir build --output-on-failure
```

5. Package local artifact:

```bash
./scripts/package_local.sh build dist
```

## Artifact Naming Contract

Release assets and local packaging output follow:

- `orc-plugin_skeleton_passthrough_linux.so`
- `orc-plugin_skeleton_passthrough_macos.dylib`
- `orc-plugin_skeleton_passthrough_windows.dll`

This aligns with Decode-Orc host registry/import expectations from Phase 7C/7D.

## CI and Release

- Unified workflow: `.github/workflows/ci.yml`
	- on push/PR: builds/tests/packages on Linux and macOS via Nix (`nix develop`), plus native Windows build; uploads CI artifacts
	- on tag push (`v*`): runs the exact same build/test/package matrix, then publishes those artifacts to GitHub Release assets

## Tagging a Release

First, check existing tags to avoid conflicts:

```bash
git tag --list
```

Then, tag a commit with a version matching the pattern `v*` (e.g., `v1.0.0`):

```bash
git tag v1.0.0
git push origin v1.0.0
```

This triggers the release workflow, which:
1. Builds and tests across Linux, macOS, and Windows
2. Packages platform-specific artifacts
3. Creates a GitHub Release with the tagged artifacts attached

## License

GPL-3.0-or-later
