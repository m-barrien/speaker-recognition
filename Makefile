all: compile

compile:
	gcc -o capture  main.c -lasound
run:
	./capture