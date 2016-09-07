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

#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <string.h>

#include "generalData.h"
#include "rleFile.h"

class GameOfLife
{
    private:
        WorldData m_worldData;
        
        uint m_numBlocks;
        uint m_numThreads;
        
        GLuint m_colorBuffer;
        GLuint * m_colorData;//[];//[m_NUM_CELLS * 2];
        GLuint * m_tmpColorData; // TODO for debugging
        
        GLuint * d_colorData;
        GLuint * d_nextColorData; // for non-OpenGL case
        
        bool m_useOpenGL;
        
    private:
        void _initialize(uint width, uint height, bool data[], bool useOpenGL);
        
    public:
        GameOfLife(uint width, uint height, bool data[]);
        GameOfLife(uint width, uint height, bool data[], bool useOpenGL);
        ~GameOfLife();
        void update();
        GLuint * getColorBuffer();
        GLuint * getColorData();
        WorldData * getWorldData();
        void loadRLE(std::string filename);
        void retrieveRLEData(RLE * rel);
};

#endif
