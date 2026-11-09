# PПLTSAЯI 2: Russki Mir

A satirical roguelike set in Moscow, 2025. You are a pultsari (a street drunk). Collect empty bottles, avoid the politsija, gopniks and FSB, get hold of a propusk and make your way into Vova's wine cellar inside the Kremlin. The game text is in Finnish, with Russian thrown in.

## Build

Needs a C++14 compiler and `make`.

    make
    ./pultsari --rt --los --anims --path

`./pultsari --help` lists all options.

## Play in the browser

Needs Node.js. The server runs the game in a pseudo-terminal and streams it to the page.

    npm install
    npm start

Then open http://localhost:3000.

## Controls (real-time mode)

| Key | Action |
| --- | --- |
| W A S D or I J K L | Move. Walking into someone hits them; walking onto an item picks it up. |
| Tab | City map (browser); in the terminal, the inventory |
| Q | Open the inventory |
| W / S (inventory) | Select an item |
| E (inventory) | Use: drink, eat or draw the pistol |
| F (inventory) | Throw: starts aiming |
| F | Choose something to throw |
| Space | Aim the pistol (if you have one); Space again fires |
| A / D (aiming) | Turn the aim |
| Space (aiming) | Throw or shoot |
| ESC | Cancel or close |
| Z | Quit |

## Layout

    include/   C++ headers
    src/       C++ sources
    web/       browser client (Node.js server + xterm.js)
    basic/     the original 1991 QuickBasic version
    test/      tests

## History

The original Pultsari was written in QuickBasic 4.5 at the turn of 1991–1992 by Jari Maaranen, Tero Maaranen and Kimmo Korhonen. The original sources are in `basic/`: `pulsubin.bas` is the original binary-format file and `pultsari.bas` is a lightly refactored ASCII version. They can be run with [QB64](https://qb64.com/) or with QuickBasic 4.5 in DOSBox.

The game was later ported to C++, and that port became PПLTSAЯI 2.
