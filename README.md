# BoostClient

BoostClient is a work-in-progress Minecraft launcher fork based on the official
[Prism Launcher source](https://github.com/PrismLauncher/PrismLauncher). It has
an original BoostClient icon and a dark teal-accented interface theme. It is an
independent, modified project and is not affiliated with or endorsed by Prism
Launcher.

The upstream launcher architecture and capabilities are retained as the
starting point, including separate instances, Minecraft version management,
Java configuration, mod loaders, and modpack import. These capabilities have
not yet been build- or runtime-verified in BoostClient.

## Get the source

This checkout preserves Prism Launcher's upstream Git history and remote. Fetch
the pinned submodules before configuring a build:

```powershell
git submodule update --init --recursive
```

## Project layout

- `launcher/` contains the Qt/C++ app, Minecraft/version and Java handling,
  account/authentication code, mod-platform support, and UI.
- `launcher/ui/` contains the Qt Widgets windows, dialogs, pages, and theme
  system. `launcher/ui/themes/BoostTheme.*` adds the BoostClient palette and
  styling without replacing launcher workflows.
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
- Prism-specific news, updater, bug-tracker, community, and translation links
  are disabled. The upstream metadata endpoint remains configured because the
  launcher uses it to retrieve version and loader metadata; replace it with a
  compatible service before operating an independent public service. The
  legacy FML library download URL also still points to Prism's host for
  compatibility with old Minecraft versions.
- A Windows Release executable was built and its `--help` smoke test succeeded on the original workstation before the cloud migration; that executable was not built in Colab.
- The previous Windows Release test run passed 22 of 23 tests. `FileSystem` failed with exit code `0xc0000409`; tests have not yet been run in this cloud copy.
- Colab is Linux and cannot produce a native Windows `.exe` with this project’s Windows/MSVC/Qt configuration. Use the Windows 2022 GitHub Actions runner after pushing this modified fork to an authorized repository.
- The BoostClient theme and identity changes are a first rebrand pass; the
  inherited Qt layout has not been redesigned or visually validated yet.

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
