# Simple IR VST

A minimal impulse response loader: pick an IR audio file from disk, blend it in
with one knob. VST3 + AU (macOS) and VST3 (Windows), plus a standalone app.

[![Build](https://github.com/leocb/Simple-IR-VST/actions/workflows/build.yml/badge.svg)](https://github.com/leocb/Simple-IR-VST/actions/workflows/build.yml)

![IR Loader editor](docs/editor.png)

## Download & install

Grab the latest builds from the
[**Releases page**](https://github.com/leocb/Simple-IR-VST/releases/latest).
Every release is built automatically on macOS and Windows by GitHub Actions.

### macOS

1. Download `Simple-IR-VST-<version>-macOS-universal.zip` and unzip it.
2. Double-click **Install.command**.
   If macOS blocks it as an unidentified developer, right-click the file and
   choose **Open**.
3. Restart your DAW and rescan plugins if needed.

The installer copies:

- `~/Library/Audio/Plug-Ins/VST3/IR Loader.vst3`
- `~/Library/Audio/Plug-Ins/Components/IR Loader.component` (AU)

The standalone app (`IR Loader.app`) is in the zip; drag it to `/Applications`
if you want to keep it. To install manually, drag the two plugin bundles into
those folders and clear the download quarantine flag:

```sh
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"IR Loader.vst3"
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"IR Loader.component"
```

Requires macOS 11 or newer. The build is Universal (Apple Silicon and Intel),
ad-hoc signed but not notarized.

### Windows

1. Download `Simple-IR-VST-<version>-Windows-x64.zip` and unzip it.
2. Right-click **Install.ps1** and choose **Run with PowerShell**.
   If script execution is blocked, open a terminal in the folder and run:

   ```powershell
   powershell -ExecutionPolicy Bypass -File .\Install.ps1
   ```

   This installs to `%LOCALAPPDATA%\Programs\Common\VST3\IR Loader.vst3`.
3. For an all-users install, run the script with `-System` from an
   administrator PowerShell; it installs to
   `C:\Program Files\Common Files\VST3`.

The standalone `IR Loader.exe` is in the zip. Windows 10/11 64-bit; the binaries
are not code-signed.

## Features

- **Load an IR from a file** — click the panel or drag a `.wav`, `.aif`, `.aiff`
  or `.flac` file onto it. Mono and stereo IRs work; IRs are resampled to the
  host sample rate automatically.
- **Mix knob (0–100%)** — 0% passes the dry input, 100% is fully convolved.
- **Zero latency** convolution (`juce::dsp::Convolution`).
- **Session recall** — the loaded IR path is saved with the plugin state and
  reloaded when the session opens.
- **Transparent when empty** — before an IR is loaded the plugin passes audio
  through untouched.
- **IR file info** — sample rate, channel format and length are shown under the
  file name.

## Build

Requirements: CMake 3.22+ and either Xcode command line tools (macOS 11+) or
Visual Studio 2022 (Windows 10/11, x64).

### macOS

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

JUCE 9 is expected in `third_party/JUCE`. If the folder is missing, CMake
downloads it automatically during configure.

Builds are native-architecture by default. For a universal binary:

```sh
cmake -S . -B build-universal -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-universal --parallel
```

### Windows

Use a Developer PowerShell for Visual Studio (2022 or newer) so that `cl.exe`
is on the PATH, then:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The project always builds VST3 and the standalone app. AU is added
automatically on macOS only.

### Outputs

By default the built plugins are copied into the current user's plugin folders
(set `-DIRLOADER_COPY_PLUGINS_AFTER_BUILD=OFF` to disable). A normal build
installs:

- macOS: `~/Library/Audio/Plug-Ins/VST3/IR Loader.vst3` and
  `~/Library/Audio/Plug-Ins/Components/IR Loader.component` (AU)
- Windows: `%LOCALAPPDATA%\Programs\Common\VST3\IR Loader.vst3`

The standalone app and release bundles are also left in the build tree:

- macOS: `build/IRLoader_artefacts/Release/Standalone/IR Loader.app`
- Windows: `build\IRLoader_artefacts\Release\Standalone\IR Loader.exe`
- Windows VST3: `build\IRLoader_artefacts\Release\VST3\IR Loader.vst3`

### Release packages

```sh
scripts/package-release.sh 1.0.1                 # macOS, from build-universal/
powershell -File scripts/package-release.ps1 -Version 1.0.1   # Windows
```

Each script puts a zip in `dist/` containing the plugins, the installer and a
short readme. The GitHub Actions workflow
([`.github/workflows/build.yml`](.github/workflows/build.yml)) builds both
platforms on every push, runs the tests, and attaches both zips to the release
when a `v*` tag is pushed.

## Tests

The test binary drives the processor offline and does not need a DAW:

```sh
cmake --build build --target IRLoaderTests
./build/IRLoaderTests
```

On Windows the binary is `build\IRLoaderTests.exe` (after the same Ninja
Release build). The same tests run on both platforms in CI.

It checks that the plugin is transparent without an IR, that 0/50/100% mix
produce the expected dry/wet results, hot-swapping an IR while audio runs,
state save/restore, and error handling for missing/invalid files. To render the
editor previews used in `docs/`:

```sh
./build/IRLoaderTests --render-ui /tmp/ir-loader-preview
```

The macOS VST3 and AU also pass `pluginval --strictness-level 10` (including
editor and automation tests) and `auval`.

## Notes

- Impulse responses are normalised by JUCE's energy-based normalisation, which
  keeps levels consistent between different IRs and leaves headroom. If an IR
  sounds quiet, raise the level after the plugin; there is deliberately no
  output gain control.
- Only the IR file path is stored in the plugin state, not the audio data. If
  the file is moved or deleted, the plugin falls back to pass-through until an
  IR is loaded again.
- The mix is smoothed over 20 ms to avoid zipper noise when automating it.

## Project layout

```
CMakeLists.txt                  build + plugin targets (macOS and Windows)
Source/PluginProcessor.*        convolution engine, mix parameter, state
Source/PluginEditor.*           file panel, drag & drop, mix knob UI
Tests/ProcessorTests.cpp        offline DSP/state tests and UI snapshot renderer
Packaging/macOS/                installer + readme for macOS releases
Packaging/Windows/              installer + readme for Windows releases
scripts/package-release.sh      builds the macOS release zip
scripts/package-release.ps1     builds the Windows release zip
.github/workflows/build.yml     CI: macOS + Windows builds, tests, releases
third_party/JUCE/               JUCE framework (downloaded, not tracked)
```

## Licence

AGPL-3.0 — see [LICENSE](LICENSE). JUCE 9 is used under its AGPLv3 option. If
you have a commercial JUCE licence you can relicense this project accordingly.
