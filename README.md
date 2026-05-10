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

## SDK Contract (Current)

- Include only the public SDK umbrella header in plugin/stage code:

```cpp
#include <orc/plugin/orc_plugin_sdk.h>
```

- Export the two required entrypoints:
	- `orc_get_stage_plugin_descriptor`
	- `orc_register_stage_plugin`

- Set descriptor versions from SDK constants:
	- `orc::kStagePluginHostAbiVersion`
	- `orc::kStagePluginApiVersion`

## Plugin Version Source

- The plugin descriptor version is set at CMake configure time.
- Default value is `project(VERSION ...)` from `CMakeLists.txt`.
- In GitHub Actions tag builds, tags like `v0.2.5` are automatically mapped to plugin version `0.2.5`.
- You can always override manually with:

```bash
cmake -S . -B build -DORC_PLUGIN_VERSION=1.2.3
```

## Preview Hook

This skeleton stage includes a minimal field/frame preview implementation to show how external transform plugins integrate with Decode-Orc's preview dialog.

In `SkeletonPassthroughStage`, the preview integration is exposed through the `PreviewableStage` interface:

- `supports_preview()`
  - Returns `true` to advertise that preview is available.
- `get_preview_options()`
  - Returns the preview modes exposed to the GUI (for example `field` and `frame`), including dimensions and item counts.
- `render_preview(option_id, index, hint)`
  - Renders and returns an RGB preview image for the selected mode/index.

Execution and preview are connected by a small cache:

- `execute(...)` passes input artifact `0` through unchanged
- The same input is cached as a `VideoFieldRepresentation`
- Preview methods read from that cached representation when the GUI requests field/frame previews

This is intentionally simple (grayscale luma rendering and basic field-pair frame weaving) so third-party authors can copy the pattern and replace only the rendering logic needed by their stage.

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
