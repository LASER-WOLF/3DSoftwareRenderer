build:
	gcc -Wall -Wfatal-errors -std=c99 ./src/*.c -lSDL2 -lm -o 3drenderer

run:
	./3drenderer

clean:
	del "3drenderer"
