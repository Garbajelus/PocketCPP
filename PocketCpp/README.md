# PocketCpp UCI Chess Engine

A small standalone C++ chess engine that speaks the Universal Chess Interface (UCI). It is designed to run against chess GUIs and other engines.

## Features

- UCI commands: `uci`, `isready`, `ucinewgame`, `position`, `go`, and `quit`
- Legal move generation with check detection
- Castling, en passant, and all four promotion choices
- FEN loading and coordinate move parsing
- Iterative-deepening alpha-beta search with repetition-aware terminal handling
- Move ordering for captures, checks, and promotions
- Evaluation for development, mobility, king safety, central control, and passed/advanced pawns
- Small varied opening book for principled early development
- ASCII engine-versus-engine self-play mode
- No third-party libraries; the recommended Windows build statically links the GCC runtimes

## Build with g++

From this folder:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -static -static-libgcc -static-libstdc++ main.cpp -o pocketcpp.exe
```

On Windows with MinGW, add the folder containing `g++.exe` to `PATH`, or use its full path in the command above.

## Run a protocol smoke test

```powershell
'uci', 'isready', 'position startpos', 'go depth 3', 'quit' | .\pocketcpp.exe
```

The output should include `uciok`, `readyok`, and a legal `bestmove`, such as `bestmove e2e4`.

The engine uses a small built-in opening book for the first few plies. It selects among several sound development lines per process, then hands the game to the search. It is a fixed book, not a system that learns from completed games.

## Run a self-play game

The executable can play a short game against itself and print the board using letters for pieces:

```powershell
.\pocketcpp.exe --selfplay 40 3
```

The arguments are the maximum number of plies and search depth. For example, `--selfplay 20 2` is a quicker low-quality test.

## Play against the engine in the terminal

Run the human-versus-engine mode:

```cmd
pocketcpp.exe --play 3
```

You play White. Enter moves in coordinate notation, such as `e2e4`, `g1f3`, or `e7e8q` for promotion. The engine prints its reply and the board after every move. Type `quit` to exit. The final number is search depth; `2` is faster and `4` is stronger but slower.

On Windows, `run-play.bat` starts this mode and supplies the MSYS2 runtime path automatically.

## Play with a draggable board

On Windows, launch the native C++ board:

```cmd
pocketcpp.exe --gui
```

You are White. Drag a piece to a legal square and PocketCpp replies as Black. The right panel compares material, legal-move mobility, and king safety for both sides. Press `Esc` to close the window. `run-gui.bat` launches this mode automatically.

## Use in a chess GUI

Add `pocketcpp.exe` as a new UCI engine. The GUI must launch the executable and communicate through standard input/output. Do not add console prompts or logging to stdout because UCI uses stdout for protocol messages.

This is a compact educational engine, not a tournament-strength engine. Its opening book is fixed and varied; it does not learn from completed games yet. A future extension could add a persistent opening book, transposition tables, and stronger time management.
