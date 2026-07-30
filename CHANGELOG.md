# Changelog

All notable changes to this project will be documented in this file.

The format is based on **Keep a Changelog**.

---

## [13.0] - 2026-04-30

### Highlights

* Introduced **Auto Step** mode for automatic program execution.
* Added configurable Auto Step speed.
* Added a new **General Settings** dialog.
* Improved Linux compatibility and debugger stability.

### Added

* Auto Step mode (`Ctrl+A`).
* Configurable Auto Step execution speed.
* Persistent Auto Step speed settings.
* General Settings window.

### Changed

* Improved Auto Step behavior on Linux.
* Enhanced variable display during execution.
* Updated user manual for Version 13.
* Updated project documentation.

### Fixed

* Linux keyboard raw mode handling.
* Auto Step stability issues.
* Variable display refresh.
* Windows/MSYS Makefile compatibility.
* Various usability improvements.

---

## [12.0] - 2026-03-30

### Highlights

* Improved symbol navigation.
* Added MinGW64 support.
* Enhanced debugger configuration.

### Added

* Scroll support in the Symbols window.
* MinGW64 compatibility.

### Changed

* Improved terminal initialization.
* Updated project documentation.
* Improved debugger startup sequence.

### Fixed

* Minor Linux terminal issues.
* General stability improvements.

---

## [11.0] - 2025-12-06

### Highlights

* Powerful source code navigation.
* Variable inspection using the mouse.
* Persistent breakpoints.

### Added

* Find Text (`Ctrl+F`).
* Go To Line (`Ctrl+G`).
* Save and restore breakpoints.
* Mouse hover to inspect variable values.
* Persistent current-line highlight color.

### Changed

* Refactored debugger internals.
* Improved Linux mouse support.
* Improved build system and packaging.
* Updated documentation.

### Fixed

* GCC 15 compatibility.
* Dynamic library loading.
* Page Down navigation.
* Linux mouse input.
* MinGW build support.

---

## [10.0] - 2025-09-22

### Highlights

* Added remote debugging support.
* Improved library loading.
* Enhanced source navigation.

### Added

* GDB Server support.
* Dynamic library loading.
* Source file loading.
* Improved Help system.

### Changed

* Improved syntax highlighting.
* Better debugger responsiveness.
* Improved source navigation.

### Fixed

* Module variable type detection.
* Compilation issues.
* Help window display.

---

## [9.0] - 2025-05-30

### Highlights

* Multiple debugger windows.
* Variable editing.
* Improved debugging workflow.

### Added

* Window switching (Windows/X11).
* Variable editing.
* Toggle variable display (`T`).

### Changed

* Improved Help window.
* Better Linux timing behavior.
* Improved debugger layout.

### Fixed

* Source file loading.
* Help window sizing.
* Window resizing issues.

---

## [8.0] - 2024-09-13

### Highlights

* Direct executable debugging.
* Improved variable editing.
* Better Linux performance.

### Added

* Debug executable directly.
* Variable editing improvements.
* Enhanced numeric formatting.

### Changed

* Improved Linux performance.
* Window resize support.
* Updated documentation.

### Fixed

* Windows keyboard handling.
* Compilation warnings.
* Numeric display issues.

---

## [7.0] - 2024-02-21

### Highlights

* Initial breakpoint support.
* Interactive variable editing.
* New Help system.

### Added

* Initial breakpoint support.
* Set Variable command.
* Variable display window.
* Application version display.
* Help window.

### Changed

* Improved Makefile.
* Better handling of invalid COBOL source files.
* Improved debugger initialization.

### Fixed

* Program rerun detection.
* Windows path handling.
* Source parsing improvements.

---

## [6.0] - 2023-12-31

### Highlights

* Mouse support.
* Interactive breakpoints.
* Improved debugger usability.

### Added

* Mouse support.
* Mouse-created breakpoints.
* Realpath support for Windows.
* Improved Help system.
* Initial watch variable improvements.

### Changed

* Improved source parser.
* Better Windows compatibility.
* Performance improvements.

### Fixed

* Windows `realpath()` implementation.
* Source loading issues.
* General stability improvements.

---

## [5.0] - 2023-11

### Highlights

* Expanded debugger functionality.

### Added

* Enhanced variable inspection.
* Debugging improvements.
* Additional keyboard shortcuts.

### Changed

* Improved debugger workflow.
* Better user interface responsiveness.

### Fixed

* Various bug fixes and stability improvements.

---

## [4.0] - 2023-10

### Added

* Activity improvements for debugger execution.
* Better source code navigation.
* Initial usability enhancements.

### Changed

* Internal debugger architecture improvements.

---

## [3.0] - 2023-09

### Added

* Initial public debugger features.
* Improved COBOL source visualization.

---

## [2.0] - 2023-07

### Added

* First complete debugger prototype.
* Initial execution control.
* Early variable inspection support.

---

## [1.0] - 2023-04-09

### Added

* Initial public project.
* Basic debugger architecture.
* GDB integration.
* Source code visualization.
* Initial terminal interface.
