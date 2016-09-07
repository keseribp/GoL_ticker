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

#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cuda_gl_interop.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>

#include "generalData.h"

class Renderer
{
    private:
        WorldData * m_worldData;
    
        GLuint m_vertexArrayBuffer;

        GLuint programID;

        GLuint m_vertexBuffer;
        GLfloat m_vertexData[2 * 4];

        GLuint m_positionBuffer;
        GLfloat * m_positionData;

        GLuint * m_colorBuffer;
        GLuint * m_colorData;

    private:
        void _initialize();
        GLuint _loadProgram(const char * vertex_file_path, 
                            const char * fragment_file_path);

    public:
        Renderer(WorldData * worldData, 
                 GLuint * colorBuffer, 
                 GLuint * colorData);
        ~Renderer();
        void display();
};

#endif
