/*
Game of Life ticker from text.
Copyright (C) 2016  Brad Parker Keserich

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GENERALDATA_H
#define GENERALDATA_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

//TODO should probably just have all the types we need in here and then include this in all the relevant files, or would that duplicate too many things?

typedef unsigned int uint;

static const uint WINDOW_X = 1300;
static const uint WINDOW_Y = 600;
static uint WIDTH = 19;
static uint HEIGHT = 19;
//GLFWwindow * window;

struct WorldData
{
    uint width;
    uint height;
};

#endif
