make : pond.c
	mkdir -p bin
	gcc pond.c -lcurses -o bin/pond

install: bin/pond
	cp bin/pond /usr/local/games/pond

uninstall:
	rm /usr/local/games/pond
	rm bin/pond
	rmdir bin
