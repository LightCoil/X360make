# x360make

A lightweight, cross-platform build system for Xbox 360 homebrew projects.

This tool supports online GitHub builds only, featuring:

- ✅ Automatic dependency management (incremental compilation, header checks)
- ✅ ZIP download & extraction (with path-traversal protection)
- ✅ GCC/LD integration (compile/link `.cpp` files to `main.elf`)
- ✅ XEX packing (wrap ELF into valid Xbox 360 executable)
- ✅ Asynchronous logging (rotating log files, UTF‑8/BOM support)
- ✅ Locale support (translation JSON files)
- ✅ GUI front-end (Win32 interface with log view and progress)

---

## 📦 Features

| Area           | Details                                                                 |
|----------------|-------------------------------------------------------------------------|
| Build System   | Online GitHub ZIP fetch + offline compiler toolchain                    |
| Logging        | Asynchronous, thread-safe, wide-character console and file logging      |
| Downloader     | WinHTTP with resume & retry logic (signature verification stubbed)      |
| Unzip Engine   | Path validation, symlink protection, canonical path checks              |
| Compilation    | Incremental GCC compile + response file link                            |
| ELF to XEX     | External `x360make_pack.exe` wrapper with process control               |
| Localization   | JSON translation files (UTF‑8 → UTF‑16), fallback-safe lookup           |
| GUI            | Modern DPI-aware Win32 GUI with ListView log, progress bar, and themes  |

---

## 🛠️ Prerequisites

- Windows 7 or newer (x64)
- Visual Studio 2019–2022
- C++17 or later (`/std:c++17`)
- Libraries:
  - `fmt` (formatting)
  - `nlohmann/json`
  - `libzip`
  - `winhttp.lib`, `crypt32.lib`, `comctl32.lib`

---

Then open `x360make.sln` in Visual Studio 2019 or 2022.

### 🔧 Build Instructions

1. **Set Configuration:**  
   In the toolbar, select `Release | x86` or `Debug | x86`.

2. **Configure Language Standard (if needed):**  
   - Right-click on `x360make` → **Properties**
   - Go to **C/C++** → **Language**
   - Set **C++ Language Standard** to `C++17` or later (`/std:c++17` or `/std:c++20`)

3. **Linker Dependencies:**  
   Ensure the following system libraries are linked:
   - `winhttp.lib`
   - `crypt32.lib`
   - `comctl32.lib`
   - `ws2_32.lib` (optional, for future networking features)

4. **Additional Include Paths:**  
   Add paths to:
   - `nlohmann/json.hpp`
   - `fmt/format.h`
   - `libzip` headers

5. **Build Solution:**  
   Press **Ctrl+Shift+B** or click **Build → Build Solution**

---

## 🖥️ GUI Overview

| Element         | Description                                                |
|------------------|------------------------------------------------------------|
| Radio Buttons   | Select Online (GitHub) or Offline (currently disabled)     |
| Edit Box        | Enter GitHub repo URL (e.g. `https://github.com/user/repo`)|
| Build Button    | Starts downloading + compiling the project                 |
| Cancel Button   | Cancels current build and terminates any external tools    |
| Progress Bar    | Visual feedback (0–100%)                                   |
| Status Text     | Shows current phase/status message                         |
| Log View        | Displays real-time log messages in ListView                |

---

## ⚠️ Known Limitations

- Only **GitHub Online Build** is supported (offline mode disabled)
- Signature verification is **stubbed** (always returns success)
- XEX packing relies on external executable (`x360make_pack.exe`)
- Localization is hardcoded to English (`lang/lang_en.json`)
- No support for `.vcproj`/MSBuild projects (only `.cpp` source folders or Makefile)

---

## ✅ Future Roadmap

- ✅ Full GUI progress tracking with thread-safe updates  
- ⬜ Real digital signature verification using `WinVerifyTrust`  
- ⬜ Optional offline build with drag-and-drop folder support  
- ⬜ Custom build profiles (release/debug, optimization flags)  
- ⬜ Built-in disassembler/debugger frontend  

---

## 📜 License

This project is licensed under the **MIT License**.  
See the `LICENSE` file for full details.

---
### ❤️ Support the Project

If you find **x360make** useful and want to support its development, you can do so here:

[![Support on Boosty](https://img.shields.io/badge/Support%20on-Boosty-orange?style=flat-square&logo=boosty)](https://boosty.to/lightcoil)

Your support helps keep both the project — and the developer — alive and coding 🐧☕  
Thank you! 🙏


