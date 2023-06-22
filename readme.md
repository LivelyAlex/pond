# Pond, a soothing in-terminal idle screen
A software that simulates (with scientific attention to detail) a little pond, complete with flowering lilypads. If you observe very quietly, you might just spot some wildlife!

## Screenshots
Example of a pond:
![pond](images/pond.png)

Hi !
![frog](images/frog.png)

## Features
- frogs
- non-resizable window

## How to build
You need the curses library to build this project. On Debian-based distributions, you can install it with `sudo apt install ncurses-dev`.

Then, `sudo make install` and you should be able to run the program with `pond`. You can also just `make` and run it with `bin/pond`.

If you're on another OS, you're on your own, sorry.
