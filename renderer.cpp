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

#include "renderer.h"

Renderer::Renderer(WorldData * worldData, GLuint * colorBuffer, GLuint * colorData)
{
    m_worldData = worldData;
    m_colorBuffer = colorBuffer;
    m_colorData = colorData;
    _initialize();
}

Renderer::~Renderer()
{
    delete[] m_positionData;
    glDeleteBuffers(1, &(*m_colorBuffer));
    glDeleteBuffers(1, &m_positionBuffer);
    glDeleteBuffers(1, &m_vertexBuffer);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &m_vertexArrayBuffer);
}

void Renderer::_initialize()
{
    uint width = m_worldData->width;
    uint height = m_worldData->height;
    uint numCells = width * height;

    glGenVertexArrays(1, &m_vertexArrayBuffer);
    glBindVertexArray(m_vertexArrayBuffer);

    programID = _loadProgram("vertexShader.glsl", "fragmentShader.glsl");

    float cellHalfExtentX = 1.0f / static_cast<float>(width);
    float cellHalfExtentY = 1.0f / static_cast<float>(height);
    GLfloat m_vertexData_new[2 * 4] = { 
         -cellHalfExtentX, -cellHalfExtentY,
          cellHalfExtentX, -cellHalfExtentY,
         -cellHalfExtentX,  cellHalfExtentY,
          cellHalfExtentX,  cellHalfExtentY
    };
    for (uint i = 0; i < 2 * 4; i++)
        m_vertexData[i] = m_vertexData_new[i];
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertexData), m_vertexData, GL_STATIC_DRAW);

    float xSep = 2.0f / static_cast<float>(width);
	float ySep = 2.0f / static_cast<float>(height);
	float xInit = -1.0f + cellHalfExtentX;
	float yInit = -1.0f + cellHalfExtentY;
	m_positionData = new float[2 * numCells];
	for (uint j = 0; j < height; j++)
        {
	    for (uint i = 0; i < width; i++)
        {
	        m_positionData[2 * height * i + 2 * j] = xInit + i * xSep;
            m_positionData[2 * height * i + 2 * j + 1] = -yInit - j * ySep;
	    }
    }
	
    glGenBuffers(1, &m_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, numCells * 2 * sizeof(GLfloat), m_positionData, GL_STATIC_DRAW);

    glGenBuffers(1, &(*m_colorBuffer));
    glBindBuffer(GL_ARRAY_BUFFER, (*m_colorBuffer));
    glBufferData(GL_ARRAY_BUFFER, numCells * 2 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numCells * sizeof(GLuint) * 2, m_colorData);
    cudaGLRegisterBufferObject((*m_colorBuffer));//TODO do here?
}

void Renderer::display()
{
    glUseProgram(programID);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glVertexAttribPointer(
	    0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
	    2,                  // size
	    GL_FLOAT,           // type
	    GL_FALSE,           // normalized?
	    0,                  // stride
	    (void*)0            // array buffer offset
    );

    // 2nd attribute buffer : positions of particles' centers
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);
    glVertexAttribPointer(
	    1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
	    2,                                // size : x + y + z + size => 4
	    GL_FLOAT,                         // type
	    GL_FALSE,                         // normalized?
	    0,                                // stride
	    (void*)0                          // array buffer offset
    );

    // 3rd attribute buffer : particles' colors
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, (*m_colorBuffer));
    glVertexAttribPointer(
	    2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
	    2,                                // size : r + g + b + a => 4
	    GL_UNSIGNED_INT,                 // type
	    GL_FALSE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
	    0,                                // stride
	    (void*)0                          // array buffer offset
    );

    glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
    glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
    glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_worldData->width * m_worldData->height); //TODO optimize with var

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

GLuint Renderer::_loadProgram(const char * vertex_file_path,const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}else{
		printf("Impossible to open %s.\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path); //TODO verbose
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path); //TODO verbose
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n"); //TODO verbose
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}
