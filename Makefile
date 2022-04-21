all: gl2.bin gl2.ref gl3.bin gl3.ref
gl2.bin: gl2.cpp glimpl.cpp glut.cpp
	g++ -o$@ -std=c++11 $^ -lX11
gl2.ref: gl2.cpp
	g++ -o$@ -std=c++11 $^ -lGL -lglut
gl3.bin: gl3.cpp glimpl.cpp glut.cpp
	g++ -o$@ -std=c++11 $^ -lX11
gl3.ref: gl3.cpp
	g++ -o$@ -std=c++11 $^ -lGL -lglut
