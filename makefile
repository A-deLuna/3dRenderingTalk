default:
	clang++ main.cc -I/usr/include/glm -I/usr/include/SDL2 -I/usr/include/assimp -lSDL2 -lassimp -O0

release:
	clang++ main.cc -I/usr/include/glm -I/usr/include/SDL2 -I/usr/include/assimp -lSDL2 -lassimp -O3
dbg:
	clang++ main.cc -I/usr/include/glm -I/usr/include/SDL2 -I/usr/include/assimp -lSDL2 -lassimp -O0 -g
