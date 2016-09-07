# GoL_ticker
Generate a Game of Life ticker from text.

Build from terminal with the following command:
$ nvcc -std=c++11 rleFile.cpp ticker.cpp gameOfLife.cu renderer.cpp main.cpp -o ticker -lglfw3 -lGLU -lGL -lGLEW -lX11 -lXxf86vm -lXinerama -lXcursor -lrt -lm -lXrandr -lpthread -lXi -ldl
