# Pond, a soothing in-terminal idle screen
A software that simulates a little pond, complete with flowering lilypads and adorable little frogs jumping around.

## Screenshots
Example of a pond:

![pond](images/pond.png)

The frog says hi!

![frog](images/frog.png)

Video example:

<figure class="video_container">
  <iframe src="https://packaged-media.redd.it/i9nxzm3e2r7b1/pb/m2-res_480p.mp4?m=exp-edc%2FDASHPlaylist.mpd&v=1&e=1687716000&s=ff892f217455f75324345a9fe41a6683b7db8f73#t=0" frameborder="0" allowfullscreen="true"> </iframe>
</figure>

## How to use pond??
Just type `pond` after installing it and you're set! Use `q` to leave the pond.

If you want more info and some options to play with, type `pond --help`!

## Features
- frogs
- the most advanced frog AI in the market
- collect rare frogs!
- frogs have deep and uniques personalities
- please be kind with the frogs and don't scare them
- non-resizable window

## How to build

### Dependencies
In addition to the build essentials, you need the `curses` library to build this project. On Debian-based distributions, you can install them both with `sudo apt install build-essential ncurses-dev`.

### Compilation
- Unix & unix-like systems:

```bash
git clone https://gitlab.com/alice-lefebvre/pond/
cd pond
make && sudo make install
pond # enjoy!
```

If you don't want/can't `sudo`, you can also just run `make`, and run the program with `bin/pond`.

- If you're on another OS, you're on your own, sorry.
