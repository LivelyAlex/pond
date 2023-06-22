make : pond.c
	mkdir -p bin
	gcc pond.c -lcurses -o bin/pond

install: make
	cp bin/pond /usr/local/games/pond

clean:
	rm /usr/local/games/pond
	rm bin/pond
	rmdir bin
