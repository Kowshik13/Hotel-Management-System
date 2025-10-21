# Hotel Management System

A cross-platform, console-based hotel management suite written in modern C++. All business data is persisted to JSON files so the application remains portable and easy to audit. The program now provides dedicated experiences for **administrators**, **hotel managers**, and **guests**, enforcing clear role-based access rules across hotel bookings, restaurant orders, and reporting.

## Features
- Role-aware dashboards for Admin, Manager, and Guest accounts
- Hotel catalogue management with room configuration and availability tracking
- End-to-end reservation flow for guests, including restaurant orders billed to rooms
- Booking oversight with lifecycle status updates and cancellation cleanup
- Operational snapshot reporting for occupancy and revenue metrics
- Secure password storage using a deterministic FNV-1a hash demo (easy to swap for stronger algorithms)

## Role summary
| Role     | Capabilities |
|----------|--------------|
| Admin    | Full control over hotels, rooms, and bookings plus reporting. |
| Manager  | Read-only hotel and room inspection, booking oversight (status updates, cancellations), and reporting. |
| Guest    | Room reservations, restaurant orders tied to bookings, and personal profile management. |

All monetary values are displayed in Euro (EUR) using ASCII-only text to avoid platform encoding issues.

## Getting started

### Prerequisites
- A C++20 capable compiler (GCC 11+, Clang 13+, or MSVC 19.30+)
- CMake 3.20 or newer

Optional but recommended for development:
- Ninja build system for faster incremental builds

### Build and run
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/hotel-management
```

On Windows PowerShell:
```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/Release/hotel-management.exe
```

Data files are stored under `src/data/` by default. At runtime they are copied to the working directory as `data/` so you can edit or version-control them separately. To reset the application state simply delete the generated `data/` directory before starting the program again.

### Sample credentials
| Role  | Username | Password    |
|-------|----------|-------------|
| Admin | `admin`  | `admin123`  |
| Manager | `manager` | `manager123` |
| Guest | `john`   | `secret`    |

Use the admin dashboard to configure hotels and rooms. Managers inherit live operational control without direct modification access. Guests can self-register for new accounts and immediately start booking.

## Project structure
```
src/
  Main.cpp              Entry point that wires repositories and launches the UI router
  models/               Plain data structures (User, Booking, Room, etc.)
  storage/              JSON-backed repositories for persistence
  ui/                   Console user interface, screens, and shared utilities
  security/             Password hashing helpers
```

## Testing
A minimal smoke test suite lives under `tests/` and can be executed with CTest:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

## Customisation tips
- To seed different starter data, edit the JSON files in `src/data/` and delete the generated `data/` folder before the next run.
- Swap `HashPasswordDemo` in `src/security/Security.cpp` with your preferred hashing algorithm for production scenarios.
- Extend role policies by updating `src/models/Role.h` and the dashboards inside `src/ui/screens/`.

Enjoy managing your hotel operations from any system with a straightforward text interface!
