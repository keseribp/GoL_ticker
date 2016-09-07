# GoL_ticker

Generate a Game of Life ticker from text.

## Build Instructions

`$ nvcc -std=c++11 rleFile.cpp ticker.cpp gameOfLife.cu renderer.cpp main.cpp -o ticker -lglfw3 -lGLU -lGL -lGLEW -lX11 -lXxf86vm -lXinerama -lXcursor -lrt -lm -lXrandr -lpthread -lXi -ldl`


## Copyright and license

Copyright (C) 2016  Brad Parker Keserich. Code released under [the GNU GPLv3 license](https://github.com/keseribp/GoL_ticker/master/LICENSE).
