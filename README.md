x360make
A lightweight, cross‐platform build system for Xbox 360 homebrew projects.
This tool supports both offline (local) and online (GitHub) builds, featuring:

Automatic dependency management (incremental compilation, header checks)

ZIP download & extraction (with path‐traversal protection)

GCC/LD integration (compile/link .cpp files to main.elf)

XEX packing (wrap ELF into a valid Xbox 360 executable)

Asynchronous logging (rotating log files, UTF‑8/BOM support)

WinHTTP downloader (resume, HTTPS, digital‐signature check stub)

Locale support (load translation JSON files, safe lookup)

GUI front‐end (Win32 interface with progress bar and log view)

Table of Contents
Features

Prerequisites

Getting Started

Clone Repository

Configure Environment Variables

Build Instructions

Usage

Offline Build

Online Build (GitHub)

Project Structure

Module Overview

License

Features
Offline & Online Build

Offline: Recursively scans a local folder for .cpp sources, performs incremental compilation (skips unchanged files), links to produce main.elf, then packs to main.xex.

Online: Downloads a ZIP archive from a GitHub repo (branch or default), extracts it, then invokes the same offline build pipeline on the root subdirectory.

Logging

Asynchronous, thread‐safe logger with file rotation (max file size)

UTF‑8 BOM on log files, wide‐character support in console output

Configurable queue size and minimum log level

Downloader

WinHTTP‐based downloader with resume support

Secure path validation (limits to downloads/ directory)

Stub for digital‐signature verification (replace with real WinVerifyTrust)

Unzip / Path Security

Extracts ZIP entries in parallel threads (configurable max threads)

Rejects any symlink entries and prevents path‐traversal via canonicalization

Adaptive buffer sizing for I/O performance

Packer

Invokes an external x360make_pack.exe to transform main.elf → main.xex

Captures both stdout and stderr via pipes, handles partial UTF‑8 encoding

Localization

Loads <repo-root>/lang/lang_<code>.json files

Safe UTF‑8 → UTF‑16 conversion, key‐based lookup with fallback

Maximum language‐code length and character restrictions

GUI (Win32)

Radio buttons to choose “Online” or “Offline” mode

Edit boxes for URL (online) or path (offline), “Build” / “Cancel” buttons

Progress bar (0–100%), status text, and ListView for live log messages

DPI‐aware layout, Unicode‐aware console output

Prerequisites
Platform: Windows 7 or newer (64‑bit recommended)

Compiler: Visual Studio 2019‑2022 (C++17 or later)

Libraries / Dependencies:

libzip (for Unzip)

fmt (C++ formatting library)

nlohmann/json (JSON parser)

WinHTTP (built into Windows SDK)

Windows SDK (for Win32 APIs, CreateProcessW, etc.)

Optional: Drive formatted as NTFS for correct Unicode handling

Getting Started
Clone Repository
bash
Копировать
Редактировать
git clone https://github.com/<your‑username>/x360make.git
cd x360make
Configure Environment Variables
GCC / LD Paths (if you ship custom toolchain in embedded\):

X360MAKE_GCC_PATH: Path to x360make_gcc.exe (e.g. C:\x360tools\gcc.exe)

X360MAKE_LD_PATH: Path to x360make_ld.exe (e.g. C:\x360tools\ld.exe)

If not set, defaults to embedded\x360make_gcc.exe and embedded\x360make_ld.exe

Packer Path:

X360MAKE_PACKER_PATH: Path to x360make_pack.exe (default: embedded\x360make_pack.exe)

Timeout (optional):

X360MAKE_TIMEOUT_MS: Maximum time (ms) to wait for external processes (default: 120 000 ms = 2 minutes)

Make Path (for offline builds using Makefile):

X360MAKE_MAKE_PATH: Path to mingw32-make.exe (if building a project that uses GNU‐Make).

Downloads Directory:

Ensure a downloads\ directory exists (where ZIPs will be stored).

Build Instructions
Open the solution/project in Visual Studio

The main project (“x360make”) is a Win32 GUI application.

Additional static‐library projects:

locale (loads JSON translation files)

logger (AsyncFileLogger)

unzip (libzip wrapper)

downloader (WinHttpDownloader)

packer (calls external packer process)

core_build (orchestrates compile/link/pack)

Set C++ Language Standard to C++17 or later

Properties → C/C++ → Language → “C++ Language Standard” → /std:c++17 (or /std:c++20)

Linker Dependencies

zip.lib (libzip)

winhttp.lib (WinHTTP)

crypt32.lib (WinCrypt for signature stub)

Comctl32.lib (for ListView and common controls)

Ws2_32.lib (if you need any network utilities

Additional Include Directories

Path to nlohmann/json.hpp

Path to fmt headers

Build → Release (or Debug) → Build Solution

After a successful build, the following binaries will be available:

x360make.exe (main GUI executable)

x360make.dll (static libs consolidated into a DLL for logging, unzip, downloader, etc.)

embedded\x360make_gcc.exe, embedded\x360make_ld.exe, embedded\x360make_pack.exe (toolchain executables, if included)

Usage
Offline Build
Launch x360make.exe.

Select the Offline radio button.

Enter the local project folder path (the directory containing .cpp or Makefile).

Click Build.

If a Makefile is found at the root, it will run mingw32-make -C <path>.

Otherwise, it will:

Recursively scan for .cpp files (limits to 100 000 files).

Perform incremental compilation (skipping unchanged .cpps).

Write a response file out\link.rsp, linking all .o → out\main.elf.

Invoke the packer: x360make_pack.exe out\main.elf out\main.xex.

Monitor progress in the progress bar and the Log view.

The final XEX file will be saved to out\main.xex.

Online Build (GitHub)
Launch x360make.exe.

Select the Online radio button.

Enter a GitHub repository URL, for example:

https://github.com/username/repo/tree/branch_name

or https://github.com/username/repo (defaults to main/master).

Click Build.

Downloads branch_name.zip (or main.zip, master.zip) into cache\project.zip.

Extracts to cache\project\, then:

Finds the top‐level subdirectory.

Runs offline build on that directory.

Monitor progress and final status.

Resulting main.xex will be in out\main.xex.

Project Structure
bash
Копировать
Редактировать
x360make/
├── README.md
├── LICENSE
├── lang/                    # Localization files (JSON)
│   ├── lang_en.json
│   └── lang_ru.json
│
├── embedded/                # Toolchain executables (optional)
│   ├── x360make_gcc.exe
│   ├── x360make_ld.exe
│   └── x360make_pack.exe
│
├── include/                 # Public headers
│   ├── locale.h
│   ├── logger.h
│   ├── unzip.h
│   ├── downloader.h
│   ├── packer.h
│   ├── core_build.h
│   ├── gui.h
│   └── version.h
│
├── src/                     # Implementation files
│   ├── locale.cpp
│   ├── logger.cpp
│   ├── unzip.cpp
│   ├── downloader.cpp
│   ├── packer.cpp
│   ├── core_build.cpp
│   └── gui.cpp
│
├── crt/                     # Bootstrap assembly for ELF (PowerPC)
│   └── crt0.S
│
├── 3rdparty/                # Third‐party libraries (e.g., libzip)
│   ├── libzip/
│   ├── fmt/
│   └── nlohmann-json/
│
├── downloads/               # Runtime downloads (GitHub ZIPs)
├── cache/                   # Temporary cache (unzipped repos)
├── out/                     # Output folder (build artifacts: .elf, .xex, logs)
│
├── x360make.sln             # Visual Studio solution
└── x360make.vcxproj         # Main project file
Module Overview
1. locale
Locale::LoadLanguage(lang_code)

Validates lang_code length (≤ 16) and character set (A-Z, a-z, 0-9, '_', '-').

Loads lang/lang_<code>.json (or falls back to lang_en.json).

Parses JSON via nlohmann::json, converts UTF‑8 keys/values → std::wstring.

Locale::L(key)

Lookup in an unordered_map<wstring,wstring>.

Thread‐safe via a std::mutex.

Returns a reference to the stored value or a static fallback if missing.

2. logger
AsyncFileLogger

Constructor: opens UTF‑8 BOM file with _O_U8TEXT, spawns a worker thread.

Log(level, message): enqueues (level, message) pairs in a std::queue (mutex‐protected).

Worker thread:

Dequeues messages, writes "[timestamp] [LEVEL] message\n" to console (if enabled) and file.

Updates fileSize_ via fetch_add(bytes_written).

Rotates file when fileSize_ ≥ maxFileSize (close/rename → open new).

Destructor: signals shutdown, flushes queue, joins thread.

3. unzip
bool Unzip(zipPath, outDir, maxThreads)

Canonicalizes/validates zipPath and outDir (no path‐traversal “../”).

Uses libzip to list entries, rejects any entry flagged as a symlink (ZIP_STAT_SYMLINK).

Spawns up to maxThreads threads to extract files (each thread fetches next index atomically).

For each entry:

Skip directories.

Convert UTF‑8 name → std::wstring.

Create parent directories (mutex‐protected).

Verify IsSubPath(outCan, parent) (canonical lowercase on Windows).

Read in chunks (64 KB buffer) and write to disk.

4. downloader
WinHttpDownloader::Download(url, outPath, maxRetries, backoffSeconds)

Validates outPath is under downloads\ (canonicalize, lowercase compare).

Attempts up to maxRetries:

Parse URL via WinHttpCrackUrl.

Open WinHTTP session/connection/request (HTTPS if needed).

If outPath.part exists, add Range: bytes=<existingSize>- header.

Send request, check HTTP status (200 or 206).

Write to outPath.part with FILE_FLAG_WRITE_THROUGH.

On success: VerifyDigitalSignature(tempFile) (stubbed → always true), rename to final outPath.

On failure: delete partial file, sleep backoffSeconds * attempt, retry.

5. packer
Packer::Pack(elfPath, xexPath, progressCallback, outStdout, outStderr)

Validates elfPath exists, has .elf extension, correct ELF magic.

Builds command line:

css
Копировать
Редактировать
x360make_pack.exe "path\to\main.elf" "path\to\main.xex"
Spawns CreateProcessW with pipes for stdout/stderr.

Two threads (ReadPipeAsync) drain the pipes into std::wstring (handling partial UTF‑8 → UTF‑16).

Waits up to timeoutMs_ (default 120 sec).

On timeout, calls TerminateProcess.

Returns true if exit code == 0 and xexPath exists.

6. core_build
CoreBuilder (constructor)

Reads environment variables:

X360MAKE_GCC_PATH, X360MAKE_LD_PATH, X360MAKE_TIMEOUT_MS.

StartOfflineBuild(path)

If <path>/Makefile exists: calls BuildWithMake(path).

Else: calls AutoBuild(path).

BuildWithMake(path)

Validates make path via EscapeCmdArg (no dangerous chars).

Executes: CreateProcessW("make -C path"), waits, checks exit code.

AutoBuild(folder)

Ensures out/ directory exists.

Scans folder recursively for .cpp files (limit 100 000 files to avoid ZIP‐bomb).

Calls IncrementalCompile(cppFiles, outDir).

Gathers all .o in out/ (limit 10 000 files).

Writes out/link.rsp with:

arduino
Копировать
Редактировать
-T "crt\xex.ld"
"out\file1.o"
"out\file2.o"
…
-o "out\main.elf"
Invokes CreateProcessW(ld @"out\link.rsp").

On success, calls packer->Pack("out\main.elf", "out\main.xex", nullptr, packOut, packErr);

IncrementalCompile(sources, outDir)

Loads outDir\build.cache.json (JSON) → oldCache (file path → timestamp).

For each src in sources: check fs::last_write_time(src) → newCache[src].

If changed or missing in oldCache, add to toCompile vector.

Save newCache → build.cache.json.

Spawn up to min(hardware_concurrency, toCompile.size()) threads:

Each thread pops next source from a mutex‐protected std::queue.

Validates that any included header *.h exists under include\ (prevent external includes).

Builds compile command:

nginx
Копировать
Редактировать
gcc -c "source.cpp" -o "output.o" -ffreestanding -nostdlib -Iinclude  
Calls CreateProcessW, waits, checks exit code, deletes from runningProcs_ once done.

Returns true if all compilations succeed.

CancelBuild()

Sets cancelRequested_ = true.

Grabs procMutex_, iterates runningProcs_, calls TerminateProcess + CloseHandle on each.

7. gui
Win32 GUI (RunGUI/WndProc)

On WM_CREATE:

Sets per‐monitor DPI awareness.

Creates controls (radio buttons, edit boxes, “Build”/“Cancel” buttons, status text, progress bar, ListView).

Loads localization strings via Locale loc; loc.LoadLanguage("en"); (hardcoded for now).

Instantiates global logger and builder.

On “Build” click:

Reads URL or path from edit box.

Disables Build button, enables Cancel.

Clears logMessages, resets cancelRequested.

Spawns buildThread (calls either StartOfflineBuild or StartOnlineBuild on builder).

On “Cancel” click:

Sets cancelRequested = true, calls builder->CancelBuild().

Updates status text “Cancel requested…”, disables Cancel.

Custom messages (WM_BUILD_STARTED, WM_BUILD_UPDATE, WM_BUILD_FINISHED):

WM_BUILD_STARTED: show “Build started…” in status.

WM_BUILD_UPDATE: if wParam != 0, treat as percentage: update progress bar and status (e.g. “Progress: 45%”). If wParam == 0, add one log line from logMessages to ListView (wrapped in WM_SETREDRAW for performance).

WM_BUILD_FINISHED: show final status (“Build completed successfully.” or “Build failed or cancelled.”), enable Build, disable Cancel.

On WM_DESTROY:

Signals cancellation, calls builder->CancelBuild().

If buildThread.joinable(), join it.

Closes logger, posts quit message.

License
This project is released under the MIT License.

Please see LICENSE for details.
