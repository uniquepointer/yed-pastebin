CFLAGS += $(shell yed --print-cflags)
CFLAGS += $(shell yed --print-ldflags)
install:
	gcc $(CFLAGS) pastebin.c -o pastebin.so
	cp ./pastebin.so ~/.config/yed/mpy/plugins
