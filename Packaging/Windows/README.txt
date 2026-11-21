Simple IR VST — Windows x64
===========================

A minimal impulse response loader: load an IR audio file, blend it in with one
mix knob. VST3 and standalone, 64-bit Windows.

CONTENTS
--------
  IR Loader.vst3   VST3 plugin bundle (a folder - keep its contents together)
  IR Loader.exe    Standalone app
  Install.ps1      PowerShell installer for the VST3
  README.txt       This file

INSTALL
-------
Option A — installer (no admin required)

  Right-click Install.ps1 and choose "Run with PowerShell".
  If PowerShell blocks the script, open a terminal in this folder and run:

    powershell -ExecutionPolicy Bypass -File .\Install.ps1

  This copies the plugin to the per-user VST3 folder:

    %LOCALAPPDATA%\Programs\Common\VST3\IR Loader.vst3

Option B — install for all users (requires admin)

  Run PowerShell as administrator and execute:

    powershell -ExecutionPolicy Bypass -File .\Install.ps1 -System

  This copies the plugin to:

    C:\Program Files\Common Files\VST3\IR Loader.vst3

Option C — manual

  Copy the whole "IR Loader.vst3" folder to one of the VST3 folders above.
  Do not flatten it: the folder contains the actual DLL under
  Contents\x86_64-win\.

After installing, restart your DAW and rescan plugins if it does not show up.
Most DAWs scan the per-user folder; if yours only scans the system folder, use
Option B.

USAGE
-----
  - Click the big panel to choose a WAV / AIFF / FLAC impulse response, or just
    drag a file onto the plugin window.
  - MIX: 0% = dry input, 100% = fully convolved.
  - The IR path is saved with your session and reloaded when you reopen it.
  - With no IR loaded the plugin passes audio through unchanged.

NOTES
-----
  - Windows 10 or 11, 64-bit.
  - The build is not code-signed. SmartScreen may warn the first time you run
    IR Loader.exe ("More info" -> "Run anyway"). VST3 plugins are loaded by
    your DAW and normally do not trigger this prompt.
  - IRs are energy-normalised for consistent levels; use your host's gain if
    you need more level.

Source code and newer releases:
  https://github.com/leocb/Simple-IR-VST

Licence: AGPL-3.0 (JUCE 9 is used under its AGPLv3 option).
