# G DOT
### Windows Administration & Security Assessment

G DOT is a Windows administration and security assessment application built with **C++20 and Qt 6**. It provides a graphical interface for authorized system monitoring and administration, with secure communication and explicit user consent.

## Key Features

- **Graphical Interface** — Dedicated management and administration consoles.
- **Secure Communication** — TLS 1.2+ with explicit certificate trust and SHA-256 identity verification.
- **Session Security** — Cryptographically random 256-bit session tokens and constant-time token comparison.
- **System Assessment** — View system information, disk usage, and running processes.
- **Audit Logging** — Local JSONL security logs and administrator-side JSON export.
- **Request Protection** — Response size limits, request validation, and connection timeouts.
- **Privacy by Design** — No hidden persistence, credential collection, keylogging, screenshots, remote shell, or arbitrary file access.

## Requirements

- Windows 10 or Windows 11
- Visual Studio 2022 with Desktop development with C++
- CMake 3.24 or later
- Qt 6.5 or later (MSVC 64-bit)
- OpenSSL command-line tools for local test certificates

## Build Instructions

Open **Developer PowerShell for Visual Studio**.

Configure the Qt installation path:

```powershell
$env:QT_ROOT = "C:\Qt\6.8.3\msvc2022_64"
```

Configure the project:

```powershell
cmake -S . -B build `
    -G "Visual Studio 17 2022" `
    -A x64 `
    "-DCMAKE_PREFIX_PATH=$env:QT_ROOT"
```

Build the application:

```powershell
cmake --build build --config Release
```

Run the test suite:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Deploy Qt dependencies:

```powershell
& "$env:QT_ROOT\bin\windeployqt.exe" `
    --release `
    .\build\Release\G-DOT.exe
```

## Launch

Run the application:

```powershell
.\build\Release\G-DOT.exe
```

## Security Testing

For local testing, generate a dedicated TLS certificate and configure the application to trust the expected certificate identity. Restrict access to the private key and never distribute it with the application.

## Design Principles

G DOT prioritizes transparency, explicit authorization, secure communication, and minimal access. It is designed to support legitimate administration and assessment without covert monitoring or unrestricted remote control.
