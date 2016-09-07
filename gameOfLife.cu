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

#include "gameOfLife.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#define THREADS_PER_BLOCK 512

#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess) 
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

__constant__ WorldData g_worldData;

//TODO should do something similar to stencils
__global__ void gol_deviceUpdate(GLuint * prevColor, GLuint * nextColor) 
{
    uint index = threadIdx.x + blockIdx.x * blockDim.x;
    uint width = g_worldData.width;
    uint height = g_worldData.height;
    uint numCells = width * height;
    
    if (index >= numCells) return; // disallow indices too high
    
    uint x = index / height;
    uint y = index % height;
    uint x_m1 = (x - 1) % width;
    if (x - 1 == -1) x_m1 = (width - 1); // for correct modulus
    uint x_p1 = (x + 1) % width;
    uint y_m1 = (y - 1) % height;
    if (y - 1 == -1) y_m1 = (height - 1); // for correct modulus
    uint y_p1 = (y + 1) % height;
    bool s = (1 == prevColor[2 * index]); // current state //TODO make sure that we have only 1 and 0 values for prevColor
    uint n = prevColor[(2 * (height * x_m1 + y_m1)) % (2 * numCells)] //TODO redundant calculations here
             + prevColor[(2 * (height * x + y_m1)) % (2 * numCells)]
             + prevColor[(2 * (height * x_p1 + y_m1)) % (2 * numCells)]
             + prevColor[(2 * (height * x_m1 + y)) % (2 * numCells)]
             //+ prevColor[2 * (index + x + y)]
             + prevColor[(2 * (height * x_p1 + y)) % (2 * numCells)]
             + prevColor[(2 * (height * x_m1 + y_p1)) % (2 * numCells)]
             + prevColor[(2 * (height * x + y_p1)) % (2 * numCells)]
             + prevColor[(2 * (height * x_p1 + y_p1)) % (2 * numCells)];
    
    if (s && n < 2)
    {
        nextColor[2 * index] = 0;// dead
        nextColor[2 * index + 1] = 0;
    }
    else if (s && (n == 2 || n == 3))
    {
        nextColor[2 * index] = 1;// alive
        nextColor[2 * index + 1] = 1;
    }
    else if (s && n > 3)
    {
        nextColor[2 * index] = 0;// dead
        nextColor[2 * index + 1] = 0;
    }
    else if (!s && n == 3)
    {
        nextColor[2 * index] = 1;// alive
        nextColor[2 * index + 1] = 1;
    }
    else
    {
        nextColor[2 * index] = prevColor[2 * index];// previous
        nextColor[2 * index + 1] = prevColor[2 * index + 1];
    }
    nextColor[2 * index + 1] = 3;// = n;
}

__global__ void gol_deviceInit(GLuint * prevColor, GLuint * nextColor) //TODO would this be better as a flat out memcpy instead?
{
    //TODO could reduce the number of calls later by tracking live cell locations
    //TODO a memcpy of some sort on device might be faster here
    uint index = threadIdx.x + blockIdx.x * blockDim.x;
    prevColor[2 * index] = nextColor[2 * index];
    prevColor[2 * index + 1] = nextColor[2 * index + 1];
}

__global__ void gol_test()
{ 
    uint index = threadIdx.x + blockIdx.x * blockDim.x;
    printf("test index %i\n", index); //TODO verbose
}

GameOfLife::GameOfLife(uint width, uint height, bool data[])
{
    _initialize(width, height, data, false);
}

GameOfLife::GameOfLife(uint width, uint height, bool data[], bool useOpenGL)
{
    _initialize(width, height, data, useOpenGL);
}

GameOfLife::~GameOfLife()
{
    delete[] m_colorData;
    cudaFree(d_colorData);
    if (m_useOpenGL)
        cudaGLUnregisterBufferObject(m_colorBuffer);
    else
        cudaFree(d_nextColorData);
}

void GameOfLife::_initialize(uint width, uint height, bool data[], bool useOpenGL)
{
    printf("width = %i, height = %i, total = %i\n", width, height, width * height); //TODO verbose
    m_worldData.width = width;
    m_worldData.height = height;
    m_useOpenGL = useOpenGL;
    
    m_numThreads = 4 * 32; //TODO could be setting better
    m_numBlocks = ((width * height) / m_numThreads) + 1; // N / THREADS_PER_BLOCK
    printf("m_numBlocks = %i, m_numThreads = %i, total = %i\n", m_numBlocks, m_numThreads, m_numBlocks * m_numThreads); //TODO verbose
    m_colorData = new GLuint[2 * width * height];
    m_tmpColorData = new GLuint[2 * width * height]; //TODO for debugging
    for (uint j = 0; j < height; j++)
    {
        for (uint i = 0; i < width; i++)
        {
            if (data[height * i + j])
            {
                m_colorData[2 * height * i + 2 * j] = 1;
                m_colorData[2 * height * i + 2 * j + 1] = 1;
            }
            else
            {
                m_colorData[2 * height * i + 2 * j] = 0;
                m_colorData[2 * height * i + 2 * j + 1] = 0;
            }
            m_tmpColorData[2 * height * i + 2 * j] = m_colorData[2 * height * i + 2 * j]; //TODO for debug
            m_tmpColorData[2 * height * i + 2 * j + 1] = m_colorData[2 * height * i + 2 * j + 1]; //TODO for debug
            //}
        }
    }
    
    cudaMalloc((void **) &d_colorData, 2 * width * height * sizeof(GLuint)); // allocate for prevColor to be held
    if (!m_useOpenGL) cudaMemcpy(d_nextColorData, m_colorData, 2 * width * height * sizeof(GLuint), cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(g_worldData, &m_worldData, sizeof(WorldData));
}

void GameOfLife::update() 
{
    if (m_useOpenGL)
    {
        void* ptr;
        cudaGLMapBufferObject(&ptr, m_colorBuffer);
        gol_deviceInit<<<m_numBlocks, m_numThreads>>>(d_colorData, (GLuint *)ptr); //TODO need to define prevColorBuffer
        gol_deviceUpdate<<<m_numBlocks, m_numThreads>>>(d_colorData, (GLuint *)ptr);
    
        //gpuErrchk( cudaPeekAtLastError() );
        //gpuErrchk( cudaDeviceSynchronize() );
    
        //cudaMemcpy(m_colorData, d_colorData, 2 * m_worldData.width * m_worldData.height * sizeof(GLuint), cudaMemcpyDeviceToHost); //TODO debugging
        cudaGLUnmapBufferObject(m_colorBuffer);
    }
    else
    {
        gol_deviceInit<<<m_numBlocks, m_numThreads>>>(d_colorData, d_nextColorData); //TODO need to define prevColorBuffer
        gol_deviceUpdate<<<m_numBlocks, m_numThreads>>>(d_colorData, d_nextColorData);
    }
} 

GLuint * GameOfLife::getColorBuffer()
{
    return &m_colorBuffer;
}

GLuint * GameOfLife::getColorData()
{
    return m_colorData;
}

WorldData * GameOfLife::getWorldData()
{
    return &m_worldData;
}

void GameOfLife::retrieveRLEData(RLE * rle)
{
    //TODO determine if we need to use the cudaGLMapBufferObject and cudaGLUnmapBufferObject calls
    if (m_useOpenGL)
    {
        void* ptr;
        cudaGLMapBufferObject(&ptr, m_colorBuffer);
        //gol_deviceInit<<<m_numBlocks, m_numThreads>>>(d_colorData, (GLuint *)ptr);  //TODO necessary?
        cudaMemcpy(m_colorData, d_colorData, 2 * m_worldData.width * m_worldData.height * sizeof(GLuint), cudaMemcpyDeviceToHost);
        cudaGLUnmapBufferObject(m_colorBuffer);
    }
    else
    {
        cudaMemcpy(m_colorData, d_colorData, 2 * m_worldData.width * m_worldData.height * sizeof(GLuint), cudaMemcpyDeviceToHost);
    }
    
    //TODO if rle is nullptr or points to garbage, then error out
    //TODO should we check the old data and delete it if it is not yet set to nullptr?
    
    rle->x = m_worldData.width;
    rle->y = m_worldData.height;
    rle->data = new bool[rle->x * rle->y];
    
    for (uint j = 0; j < m_worldData.height; j++)
    {
        for (uint i = 0; i < m_worldData.width; i++)
        {
            if (m_colorData[2 * m_worldData.height * i + 2 * j] == 1)
                rle->data[m_worldData.height * i + j] = true;
            else
                rle->data[m_worldData.height * i + j] = false;
        }
    }
    
    printf("Neighbor Data from GPU:\n"); //TODO verbose
    for (uint j = 0; j < m_worldData.height; j++)
    {
        for (uint i = 0; i < m_worldData.width; i++)
        {
            printf("%s ",std::to_string(m_colorData[2 * m_worldData.height * i + 2 * j + 1]).c_str());
        }
        printf("\n");
    }
}
