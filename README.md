# orc-plugin_skeleton

External Decode-Orc stage plugin skeleton repository.

This repository is intended to be the canonical starter template for third-party stage plugins.

## What It Contains

- Minimal stage implementation: `skeleton_passthrough`
- Required plugin entrypoints:
	- `orc_get_stage_plugin_descriptor`
	- `orc_register_stage_plugin`
- Example stage help text (`instructions.md`) wired into the host help dialog
- Local smoke tests for stage metadata and entrypoint registration
- Cross-platform CI workflow targets for Linux, macOS, and Windows
- Release workflow that uploads platform plugin artifacts together with the
  mandatory `orc-plugin-manifest.yaml`

## SDK Contract (Decode-Orc 2.x — host ABI 12 / plugin API 4)

- Include only the public SDK umbrella header in plugin/stage code:

```cpp
#include <orc/abi/orc_plugin_sdk.h>
```

  The SDK is now tiered: `orc/abi/` holds the frozen binary contract
  (descriptor, entrypoints, registration, services), `orc/stage/` the stage
  interfaces and data types, and `orc/support/` the header/helper conveniences.
  The umbrella above pulls in everything a stage plugin needs. The former
  `<orc/plugin/orc_plugin_sdk.h>` path is a deprecated shim retained for one
  release — include the `orc/abi/` path directly.

- Export the two required entrypoints:
	- `orc_get_stage_plugin_descriptor`
	- `orc_register_stage_plugin`

- Build the descriptor with the `ORC_STAGE_PLUGIN_DESCRIPTOR` macro, which
	fills in the host ABI version, plugin API version, and toolchain tag from
	the SDK the plugin is compiled against. The host requires all three to
	match exactly at load time; plugins built against a 1.x SDK are refused.

- Declare stage metadata through `NodeTypeInfo`. Since host ABI 12 the Add
	Stage menu group is **not** plugin-declared: `NodeTypeInfo` dropped its
	`menu_category` and `sink_category` fields, and the category is derived from
	the stage's `NodeType` by `orc::stage_category_for()` (SOURCE → Source,
	TRANSFORM/MERGER/COMPLEX → Transform, ANALYSIS_SINK → Analysis, SINK →
	Sink). `NodeTypeInfo::category()` reports the result; a stage can no longer
	place itself under an invented category.

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

Since host ABI 11, such a wrapper should also override `video_passthrough_source(FrameID)` to return the wrapped source for any frame whose CVBS video content it leaves byte-identical (for example a correction pass over a frame with nothing to correct, or a stage that only appends an audio channel pair). The host uses that declaration to share frame-content-keyed stored observations with the upstream node instead of re-analysing identical samples. The answer must come from metadata alone — never decode samples to compare — and returning `nullptr`, the default, is always safe. This skeleton forwards the input pointer itself, so the host already sees a single representation and needs no declaration.

## Stage Help Hook

Decode-Orc shows per-stage documentation through `DAGStage::get_instructions()`, which returns Markdown rendered by the host help dialog. This skeleton wires it up the preferred way — a standalone `instructions.md` file rather than an inline string:

- `instructions.md` in the repository root holds the stage documentation (Markdown).
- `SkeletonPassthroughStage` adds the SDK's `ORC_STAGE_INSTRUCTIONS_MD` macro (from `<orc/plugin/orc_stage_tooling.h>`, already included via the umbrella header) to its public section. The macro implements `get_instructions()` by locating the loaded plugin library with `dladdr` / `GetModuleHandleEx` and reading the `.md` file next to it.
- `orc_add_stage_plugin()` detects `instructions.md` in the plugin source directory automatically, copies it to `lib<plugin>.md` beside the built library, and installs it with the plugin. Editing the file triggers a re-copy without rebuilding the stage.

To document your own stage, keep the macro in the class and edit `instructions.md`. For plugins that cannot ship a sidecar file, the SDK also offers the legacy `ORC_STAGE_INSTRUCTIONS(<string>)` macro to embed the Markdown inline.

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

5. Package local artifact and its manifest fragment:

```bash
./scripts/package_local.sh build dist
```

## Artifact Naming Contract

Release assets and local packaging output follow
`orc-plugin_<stage-name>_<platform>[_abi<N>].<ext>`:

- `orc-plugin_skeleton_passthrough_linux_abi12.so`
- `orc-plugin_skeleton_passthrough_macos_abi12.dylib`
- `orc-plugin_skeleton_passthrough_windows_abi12.dll`

The `_abi<N>` token records the host ABI the binary targets, so one release can
carry builds for several host ABIs side by side. The host validates downloads
against this pattern, but it no longer *selects* by name — that is the release
manifest's job (below). `package_local.sh` takes the ABI number from the built
descriptor, so the name always matches the binary.

## Release Manifest (required)

Since host ABI 12 every release must publish an `orc-plugin-manifest.yaml`
asset alongside its binaries. A release without one — or with an invalid one —
cannot be browsed, installed, or updated to: the host resolves which artifact
to install entirely from the manifest, and refuses anything whose declared ABI
or toolchain tag cannot work.

```yaml
manifest_schema: 1
plugin_id: org.decodeorc.stage.skeleton_passthrough
plugin_version: 1.0.0
artifacts:
  - file: orc-plugin_skeleton_passthrough_linux_abi12.so
    platform: linux
    abi: 12
    toolchain_tag: gcc15/libstdc++
    sha256: 5854e982866bf2d3c8211c9f34af3d5686a66036953aaf026bef33b0d5a80b7b
```

This repository generates it rather than hand-maintaining it, because the ABI
number and toolchain tag are properties of the compiler that produced each
binary:

- `tools/plugin_build_info.cpp` builds into `orc-plugin-build-info`, compiled
	with the same toolchain and SDK headers as the plugin, and prints the
	descriptor's own `plugin_id`, `plugin_version`, `abi` and `toolchain_tag`.
- `scripts/package_local.sh` reads those values, names and copies the release
	asset, digests it, and writes `dist/plugin-manifest-<platform>.yaml` — a
	complete, valid single-platform manifest.
- `scripts/merge_manifests.sh` combines the per-platform fragments into the
	single `orc-plugin-manifest.yaml` published with the release, failing if the
	fragments disagree about the plugin identity or declare no artifacts.

The manifest is a declaration by CI, not proof — the load-time ABI/toolchain
gate remains the enforcement point. Its purpose is to give the plugin browser a
definitive compatibility verdict before anything is downloaded, and to supply
the digest used to verify (and quarantine) downloads and cache hits.

## CI and Release

- Unified workflow: `.github/workflows/ci.yml`
	- on push/PR: builds/tests/packages on Linux and macOS via Nix (`nix develop`), plus native Windows build; uploads CI artifacts
	- on tag push (`v*`): runs the exact same build/test/package matrix, merges the per-platform manifest fragments, then publishes the artifacts and `orc-plugin-manifest.yaml` to GitHub Release assets
- `ORC_SDK_REF` in the workflow pins the decode-orc checkout the SDK comes
	from. It normally tracks `main`; it is currently pinned to the ABI 12 branch
	`20260729-001` and should return to `main` once that branch merges.

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
2. Packages platform-specific artifacts and their manifest fragments
3. Merges the fragments into `orc-plugin-manifest.yaml`
4. Creates a GitHub Release with the tagged artifacts and the manifest attached

## License

GPL-3.0-or-later
