CC := gcc
CFLAGS := -g -Wall -Wno-deprecated-declarations -Werror -std=c99

all: diskinfo disklist diskget diskput

clean:
	rm -rf diskinfo diskinfo.dSYM
	rm -rf disklist disklist.dSYM
	rm -rf diskget diskget.dSYM
	rm -rf diskput diskput.dSYM

diskinfo: diskinfo.c basic.c
	$(CC) $(CFLAGS) -o diskinfo diskinfo.c basic.c

disklist: disklist.c basic.c
	$(CC) $(CFLAGS) -o disklist disklist.c basic.c

diskget: diskget.c basic.c
	$(CC) $(CFLAGS) -o diskget diskget.c basic.c

diskput: diskput.c basic.c
	$(CC) $(CFLAGS) -o diskput diskput.c basic.c