make : pond.c
	mkdir -p bin
	gcc -std=gnu99 -Wall -Os pond.c -lcurses -o bin/pond

install: bin/pond
	rm -f /usr/local/games/pond #old location
	cp bin/pond /usr/games/pond

uninstall:
	rm -f /usr/local/games/pond
	rm /usr/games/pond
	rm bin/pond
	rmdir bin
