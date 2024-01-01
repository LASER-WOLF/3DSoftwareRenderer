build:
	gcc -Wall -std=c99 ./src/*.c -I"C:\SDL2\include" -L"C:\SDL2\lib" -lmingw32 -lSDL2main -lSDL2 -o renderer.exe

run:
	./renderer.exe

clean:
	del "renderer.exe"