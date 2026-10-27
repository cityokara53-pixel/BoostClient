# BoostClient

BoostClient is a work-in-progress Minecraft launcher fork based on the official
[Prism Launcher source](https://github.com/PrismLauncher/PrismLauncher). It has
an original BoostClient icon and a dark cyan-accented interface theme. It is an
independent, modified project and is not affiliated with or endorsed by Prism
Launcher.

## OG Client copyright and source-use notice

Original OG Client branding, artwork, and OG Client-specific contributions by
yourdevilpro are Copyright (c) 2026 yourdevilpro. All rights reserved. Copying
or redistributing those original materials is not permitted.

This notice applies only to original OG Client materials. The upstream Prism
Launcher code and other third-party components retain the licenses and notices
that apply to them, including the GNU GPL and the terms in `LICENSE` and
`COPYING.md`. This notice does not relicense those components.

The upstream launcher architecture and capabilities are retained as the
starting point, including separate instances, Minecraft version management,
Java configuration, mod loaders, and modpack import. The Windows Release
executable and its command-line startup have been verified; the existing
FileSystem test remains a known failure.

## Windows app-only releases

Windows app-only releases are ZIP packages, not installers. Extract the ZIP to
a permanent folder and run `boostclient.exe`; on first launch, the app creates
Desktop and Start Menu shortcuts and registers itself in Startup Apps. The ZIP
does not contain account or instance data. It is not accepted by the automatic
updater for non-portable installs, so close the app and manually replace its
files when applying a later app-only release.

## Get the source

This project preserves Prism Launcher's upstream Git history. Fetch the pinned
submodules before configuring a build:

```powershell
git submodule update --init --recursive
```

## Project layout

- `launcher/` contains the Qt/C++ app, Minecraft/version and Java handling,
  account/authentication code, mod-platform support, and UI.
- `launcher/ui/` contains the Qt Widgets windows, dialogs, pages, and theme
  system. `MainWindow.cpp` adds the BoostClient Home and Installations
  navigation while retaining the existing instance model, launch, configure,
  account, and create-installation actions. `launcher/ui/themes/BoostTheme.*`
  supplies the cyan-on-dark palette and UI styling.
- `launcher/resources/` contains Qt resource collections, built-in icon
  themes, and other embedded assets.
- `launcher/translations/` contains translation loading/model code. This source
  checkout does not include `.po`, `.qm`, or `.ts` catalogs; remote translation
  URLs are disabled until BoostClient has its own translation service.
- `program_info/` controls product metadata, app IDs, installer data, and
  platform icons.
- `tests/`, `CMakeLists.txt`, `CMakePresets.json`, and `cmake/vcpkg/` provide
  test and build configuration.

## Build on Windows

Use the official
[Prism Launcher Windows build guide](https://prismlauncher.org/wiki/development/build-instructions/)
as the dependency reference. The current CMake preset requires CMake 3.28 or
newer, Ninja Multi-Config, Visual Studio 2022 C++ tools, JDK 17, and Qt 6 with
the Qt Image Formats, NetworkAuth, and SVG components. CMake's vcpkg preset
fetches the remaining configured dependencies.

Install Visual Studio Build Tools from an **elevated PowerShell**; its C++
workload installation requires administrator access:

```powershell
winget install --source winget --exact --id Kitware.CMake --accept-source-agreements --accept-package-agreements
winget install --source winget --exact --id Ninja-build.Ninja --accept-source-agreements --accept-package-agreements
winget install --source winget --exact --id EclipseAdoptium.Temurin.17.JDK --accept-source-agreements --accept-package-agreements
winget install --source winget --exact --id Microsoft.VisualStudio.2022.BuildTools --override '--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended' --accept-source-agreements --accept-package-agreements
```

Install Qt 6.9.3 for the matching MSVC toolset and the required Qt add-on
modules. The `aqt` command is available after opening a new terminal:

```powershell
winget install --source winget --exact --id miurahr.aqtinstall
aqt install-qt windows desktop 6.9.3 win64_msvc2022_64 -O C:\Qt -m qtimageformats qtnetworkauth
```

Open an **x64 Native Tools PowerShell** with the required tools available, then
run these commands from the repository root:

```powershell
cmake --preset windows_msvc -D CMAKE_PREFIX_PATH="C:\Qt\6.9.3\msvc2022_64"
cmake --build --preset windows_msvc --config Debug
cmake --install build --config Debug
ctest --preset windows_msvc --config Debug
```

For a portable install, use:

```powershell
cmake --install build --config Debug --component portable
```

### Windows Visual C++ runtime

Windows MSVC builds keep the dynamic `/MD` runtime and install the five
Microsoft VC143 redistributable DLLs imported by the packaged launcher, Qt
libraries, and helper executables directly beside the application:
`msvcp140.dll`, `msvcp140_1.dll`, `msvcp140_2.dll`, `vcruntime140.dll`, and
`vcruntime140_1.dll`. CMake sources them from the Visual Studio redistributable
directory and fails configuration if any required file is missing. Together,
these x64 DLLs add about 1 MiB. The Windows installer includes them with the
launcher and no longer downloads or installs the separate Visual C++
Redistributable.

The launcher source and packaged native imports do not reference WebView2, and
the WebView2 Runtime is not bundled. Windows system libraries and the Universal
C Runtime supplied by supported Windows versions remain operating-system
requirements. Minecraft still needs a compatible Java runtime; BoostClient can
manage Java installations, and network access is needed for game metadata,
accounts, and downloads.

Do not reuse a build directory configured with Prism Launcher's previous
identity or API settings; configure a fresh build directory after changing
CMake options.

## Branding touchpoints

- Product, app ID, environment prefix, and display name: `program_info/CMakeLists.txt`
- Executable and service defaults: top-level `CMakeLists.txt`
- Window title and application identity: `launcher/Application.cpp` and `launcher/ui/MainWindow.cpp`
- Installer and platform metadata: `program_info/win_install.nsi.in` and the
  `program_info/org.boostclient.*` files
- App icons: `program_info/org.boostclient.BoostClient.svg`,
  `program_info/boostclient.ico`, `program_info/boostclient.icns`, and
  `program_info/BoostClient.icon/`
- Gaming theme and default theme selection:
  `launcher/ui/themes/BoostTheme.*` and `launcher/ui/themes/ThemeManager.cpp`

## Integrations and limitations

- Microsoft sign-in uses the upstream authentication implementation, but
  `Launcher_MSA_CLIENT_ID` is intentionally empty. Register and authorize a
  BoostClient application with Microsoft, then configure its client ID before
  expecting sign-in to work.
- The CurseForge key and Imgur client ID are also empty. Those integrations
  require credentials that BoostClient is authorized to use.
- Windows releases use this project's own GitHub Releases feed, not Prism
  Launcher's updater feed. Automatic update checks are enabled by default and
  install stable releases without an update confirmation; Windows setup runs
  silently using the existing per-user install location. Manual update checks
  remain available. A public repository and at least one published release are
  required for anonymous update checks and downloads.
- Download the latest stable Windows installer from
  https://github.com/cityokara53-pixel/BoostClient/releases/latest/download/BoostClient-Setup.exe.
  Each release also keeps a versioned installer and publishes checksums for both
  download names, so this link always resolves to the latest stable release.
- Pushing a `MAJOR.MINOR.PATCH` tag runs
  `.github/workflows/boostclient-release.yml`. It builds the matching versioned
  Windows installer with the updater enabled, runs tests other than the known
  interactive `FileSystem` test, and publishes both the versioned installer and
  the stable `BoostClient-Setup.exe` download alongside their SHA-256 checksums.
- Prism-specific news, bug-tracker, community, and translation links are
  disabled. The upstream metadata endpoint remains configured because the
  launcher uses it for version and loader metadata; replace it with a compatible
  service before operating an independent public service. The legacy FML
  library download URL also still points to Prism's host for compatibility
  with old Minecraft versions.
- The Home and Installations flow is an initial UI redesign. Other launcher
  workflows continue to use upstream pages and menus; a separate Drive
  publisher and end-to-end Drive release pipeline have not been implemented.

## Licensing and attribution

BoostClient is a modified fork, not a wholly original reimplementation. The
launcher code is distributed under **GPL-3.0-only**; keep the upstream
copyright notices, `LICENSE`, `COPYING.md`, and applicable third-party notices
with source and binary distributions. Components may have their own licenses.
See [`program_info/LICENSE`](program_info/LICENSE) for the upstream branding
asset license. Original BoostClient branding assets are licensed under
CC BY-SA 4.0; inherited Prism Launcher assets retain their upstream notices.

When redistributing modified source or binaries, follow the GPL terms,
including providing corresponding source and preserving required notices.
Consult the included license texts for the complete requirements.
