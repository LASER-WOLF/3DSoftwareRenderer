build:
	gcc -Wall -Wfatal-errors -std=c99 ./src/*.c -lSDL2 -lm -o 3drenderer

run:
	./3drenderer

clean:
	rm "3drenderer"

build_win:
	gcc -Wall -std=c99 ./src/*.c -I"C:\SDL2\include" -L"C:\SDL2\lib" -lmingw32 -lSDL2main -lSDL2 -lm -o 3drenderer.exe

run_win:
	./3drenderer.exe

clean_win:
	del "3drenderer.exe"
