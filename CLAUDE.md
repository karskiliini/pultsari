# CLAUDE.md - Pultsari Development Guide

## Project Overview

Pultsari is a retro text-based dungeon crawler game, originally written in QuickBASIC 4.5, ported to modern C++14. The player navigates an urban Finnish environment, interacting with NPCs, managing inventory, health, and intoxication level (promilles).

## Build System

- **Compiler:** g++ with C++14 (`-std=c++14`)
- **Build tool:** GNU Make

### Commands

```bash
make            # Build the project (produces ./pultsari binary)
make clean      # Remove build artifacts (*.o and binary)
```

### Optional ncurses UI

Uncomment these lines in `Makefile` to enable ncurses:
```makefile
CURSESFLAG := -DUSE_NCURSES
CURSESLIB := -lncurses
```

## Project Structure

```
pultsari/
├── Makefile              # Build configuration
├── README.md             # User-facing documentation
├── basic/                # Original QuickBASIC source files (reference only)
├── include/              # C++ header files
│   ├── pultsari.hpp      # Main game controller
│   ├── level.hpp         # Level/map management
│   ├── player.hpp        # Player character
│   ├── persons.hpp       # NPC base class
│   ├── building.hpp      # Building types
│   ├── item.hpp          # Item system
│   ├── input.hpp         # Input handling (vim-style: h/j/k/l)
│   ├── printer.hpp       # Abstract printer base class
│   ├── printerterm.hpp   # Terminal renderer
│   ├── printerncurses.hpp # ncurses renderer (optional)
│   ├── common.hpp        # Utilities (random, bounds, sleep)
│   ├── coord.hpp         # Template coordinate system
│   ├── direction.hpp     # Direction enum
│   ├── inventory.hpp     # Inventory management
│   ├── stats.hpp         # Player statistics
│   ├── mask.hpp          # Vision/masking system
│   ├── pathmask.hpp      # Pathfinding mask
│   ├── animation.hpp     # Animation system
│   ├── intro.hpp         # Game intro sequence
│   └── persontype.hpp    # Person type enum
└── src/                  # C++ source files
    ├── entrypoint.cpp    # Main entry point
    ├── pultsari.cpp      # Game loop and controller
    ├── level.cpp         # Map generation and management
    ├── player.cpp        # Player logic
    ├── persons.cpp       # NPC behavior
    ├── building.cpp      # Building interactions
    ├── item.cpp          # Item definitions
    ├── input.cpp         # Input processing
    ├── printerterm.cpp   # Terminal rendering
    ├── printerncurses.cpp # ncurses rendering
    ├── common.cpp        # Shared utilities
    ├── mask.cpp          # Vision calculations
    ├── pathmask.cpp      # Path calculations
    ├── animation.cpp     # Animation logic
    └── intro.cpp         # Intro sequence
```

## Architecture

- **Printer abstraction:** `Printer` is an abstract base class with `PrinterTerm` (terminal) and `PrinterNcurses` (ncurses) implementations (strategy pattern).
- **Entity hierarchy:** `Person` is the base class for all NPCs; `Building` and `Item` use inheritance for variants.
- **Coordinate system:** Template-based `Coord<T>` class for positions.
- **Feature flags (preprocessor):**
  - `ANIMATIONS_ENABLED` - Particle/text animations
  - `LOS_ENABLED` - Line of sight system
  - `USE_NCURSES` - ncurses UI backend

## Code Conventions

- **Namespaces:** Logical grouping (PlayerNS, InputNS, DirectionNS, IntroNS, VisionNS, PathNS, common)
- **Naming:**
  - Classes: `PascalCase`
  - Methods: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
- **Header guards:** `#ifndef FILE_HPP` / `#define FILE_HPP` / `#endif`
- **Includes:** Local headers in quotes, system headers in angle brackets
- **const correctness:** Applied on method signatures and parameters
- **Comments:** Minimal; only where logic is non-obvious

## CI/CD

Two GitHub Actions workflows (`.github/workflows/`):
1. **main.yml** - Builds on push/PR to master (`make clean && make`)
2. **codeql-analysis.yml** - CodeQL security scanning (push/PR + weekly schedule)

## Testing

No formal test framework. The project is tested manually by running the game. CodeQL provides static analysis for security issues.

## Git Conventions

- **Primary branch:** master
- **Commit style:** Concise, descriptive messages in sentence case
  - Feature: "Add feature to increase health every 50 steps"
  - Fix: "Fix an issue: convict and thief couldn't be caught by cops"
  - Cleanup: "minor cleaning up"

## Dependencies

- C++ standard library only (no external dependencies for default build)
- Optional: ncurses library for enhanced UI
