make : pond.c
	mkdir -p bin
	clang pond.c -lcurses -o bin/pond

install: bin/pond
	cp bin/pond ~/../usr/bin/pond

uninstall:
	rm ~/../usr/bin/pond
	rm bin/pond
	rmdir bin
