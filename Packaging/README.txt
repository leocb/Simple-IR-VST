Simple IR VST 1.0.0 — macOS (Universal: Apple Silicon + Intel)
================================================================

A minimal impulse response loader: load an IR audio file, blend it in with one
mix knob. VST3, AU and standalone.

CONTENTS
--------
  IR Loader.vst3        VST3 plugin
  IR Loader.component   Audio Unit plugin
  IR Loader.app         Standalone app
  Install.command       Double-click installer for the plugins
  README.txt            This file

INSTALL
-------
Option A — installer

  1. Double-click Install.command.
     If macOS says it cannot be opened because it is from an unidentified
     developer, right-click (or Control-click) Install.command and choose Open.

  2. The installer copies the plugins to:
       ~/Library/Audio/Plug-Ins/VST3/IR Loader.vst3
       ~/Library/Audio/Plug-Ins/Components/IR Loader.component

Option B — manual

  Drag "IR Loader.vst3" to  ~/Library/Audio/Plug-Ins/VST3/
  Drag "IR Loader.component" to  ~/Library/Audio/Plug-Ins/Components/
  (Create the folders if they do not exist.)

  Then clear the download quarantine flag:

    xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"IR Loader.vst3"
    xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"IR Loader.component"

  Dragging the app to /Applications is optional.

After installing, restart your DAW and rescan plugins if it does not show up.

USAGE
-----
  - Click the big panel to choose a WAV / AIFF / FLAC impulse response, or just
    drag a file onto the plugin window.
  - MIX: 0% = dry input, 100% = fully convolved.
  - The IR path is saved with your session and reloaded when you reopen it.
  - With no IR loaded the plugin passes audio through unchanged.

NOTES
-----
  - Builds are ad-hoc signed but not notarized, so Gatekeeper may warn the
    first time. The quarantine commands above resolve that. No Apple
    Developer ID is used.
  - IRs are energy-normalised for consistent levels; use your host's gain if
    you need more level.
  - Requires macOS 11 or newer.

Source code and newer releases:
  https://github.com/leocb/Simple-IR-VST

Licence: AGPL-3.0 (JUCE 9 is used under its AGPLv3 option).
