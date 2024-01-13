build:
	gcc -Wall -std=c99 ./src/*.c -I"C:\SDL2\include" -L"C:\SDL2\lib" -lmingw32 -lSDL2main -lSDL2 -lm -o 3drenderer.exe

run:
	./3drenderer.exe

clean:
	del "3drenderer.exe"