#!/bin/python3
LIB_FILES = ["glimpl.cpp", "glut.cpp"]
EXAMPLE_NAMES = ["gl2", "gl3"]
OUTPUT_FILE_NAME = "Makefile"

o = open("Makefile", "w")

print("all:", " ".join("{0}.bin {0}.ref".format(example) for example in EXAMPLE_NAMES),
	  file=o)

for example in EXAMPLE_NAMES:
	print("{0}.bin: {0}.cpp".format(example), " ".join(LIB_FILES), file=o)
	print("\tg++ -o$@ -std=c++11 $^ -lX11", file=o)

	print("{0}.ref: {0}.cpp".format(example), file=o)
	print("\tg++ -o$@ -std=c++11 $^ -lGL -lglut", file=o)
