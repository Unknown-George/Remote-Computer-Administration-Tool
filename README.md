# G DOT

G DOT is a visible, consent-based Windows administration and assessment
application written in C++20 with Qt 6.

## Features

- Single branded Windows executable
- G DOT graphical launcher
- Managed Computer interface
- Administration Console interface
- TLS 1.2-or-later connections
- Explicit certificate trust
- SHA-256 certificate identity verification
- Random 256-bit session token
- Constant-time token comparison
- Per-request local approval
- Windows system inventory
- Windows disk usage
- Windows process inventory
- Local JSONL security audit logs
- Administrator-side JSON export
- Request and response size limits
- Connection timeouts
- No automatic startup
- No hidden persistence
- No remote shell
- No credential collection
- No screenshots
- No keylogging
- No arbitrary file access

## Requirements

- Windows 10 or Windows 11
- Visual Studio 2022
- Desktop development with C++
- CMake 3.24 or newer
- Qt 6.5 or newer
- Matching Qt MSVC 64-bit kit
- OpenSSL command-line tools for test certificates

## Build

Open a Developer PowerShell for Visual Studio.

Set the Qt installation directory:

```powershell
$env:QT_ROOT = "C:\Qt\6.8.3\msvc2022_64"

Configure:

cmake -S . -B build `
    -G "Visual Studio 17 2022" `
    -A x64 `
    "-DCMAKE_PREFIX_PATH=$env:QT_ROOT"
Build:

cmake --build build --config Release
Run tests:

ctest `
    --test-dir build `
    -C Release `
    --output-on-failure

Deploy Qt runtime files:

& "$env:QT_ROOT\bin\windeployqt.exe" `
    --release `
    .\build\Release\G-DOT.exe
    Local certificate
Create a local certificate for testing:

New-Item -ItemType Directory -Force certs | Out-Null

openssl req `
    -x509 `
    -newkey rsa:3072 `
    -sha256 `
    -nodes `
    -keyout certs/server-key.pem `
    -out certs/server-cert.pem `
    -days 30 `
    -subj "/CN=localhost" `
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1" `
    -addext "basicConstraints=critical,CA:FALSE" `
    -addext "extendedKeyUsage=serverAuth"

    Restrict the private key:

$account = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name

icacls certs/server-key.pem `
    /inheritance:r `
    /grant:r "${account}:(R)"
Local demonstration
Start the application:

Start-Process .\build\Release\G-DOT.exe

---

## Build and run

```powershell
$env:QT_ROOT = "C:\Qt\6.8.3\msvc2022_64"

cmake -S . -B build `
    -G "Visual Studio 17 2022" `
    -A x64 `
    "-DCMAKE_PREFIX_PATH=$env:QT_ROOT"

cmake --build build --config Release

ctest `
    --test-dir build `
    -C Release `
    --output-on-failure

& "$env:QT_ROOT\bin\windeployqt.exe" `
    --release `
    .\build\Release\G-DOT.exe

.\build\Release\G-DOT.exe