prodcon: prodcon.c
	gcc -Wall -Wextra -o $@ $^ -lpthread

prodcon.zip:
	rm -f $@
	zip $@ Makefile prodcon.c
