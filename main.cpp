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

// build from command line:
// $ nvcc -std=c++11 rleFile.cpp ticker.cpp gameOfLife.cu renderer.cpp main.cpp -o ticker -lglfw3 -lGLU -lGL -lGLEW -lX11 -lXxf86vm -lXinerama -lXcursor -lrt -lm -lXrandr -lpthread -lXi -ldl

/*
TODO: (as of 03/09/16)
    - clean up the code. there are still a lot of commented out sections and a lot of printf statements that are unecessary when not debugging.
    - there should be a release version fully compiled and included in a wrapper with ImageMagik so that screen dumps may be used to make animated GIFs of the ticker. there needs to be a display-less render option here too.
    - screen dumps
    - still need to tune the thread counts automatically with some maybe trial and error to get it working in every case. in cases where the world is too large then we need to break things into quads, do some checking to see what needs to be updated, and then loop through the necessary super cells
    - iteration speed should be done differently than it is and there should be a command line option for it
    - we need a help option with descriptions at the command line
    - there needs to be a flag for the input string (or maybe that is the only non flagged input)
    - should have non-gpu render options
    - should do an analysis of the performance, especially with GPU vs other simulation methods
    - there is a lot of cleanup and optimization that may be done
    - there could be some restructuring and renaming to make things more consistent and clearer. there are also likely issues with memory cleanups and safety
    - empty belts should be exluded or kept via options at a min belt length
    - still need to test all of the symbols
    - it would be nice to have different font options
    - it might be nice to allow for right to left renders and top to bot and bot to top renders
    - it might be useful to allow for screen click and space bar pause and play
    - should maybe send a working version of this to the people who made the original Golly ticker
    - should put this project on Github
    - should put better comments and headers on all the classes
    - might be useful to allow for RLE file generation from command line as well
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <regex>
#include <thread>
#include <chrono>

#include "generalData.h"
#include "rleFile.h"
#include "ticker.h"
#include "gameOfLife.h"
#include "renderer.h"

int main(int argc, char* argv[]) // input should be in quotes
{
    GLFWwindow * window;

    // initialize GLFW
	if(!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for MacOS? otherwise should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(WINDOW_X, WINDOW_Y, "Game of Life", NULL, NULL);
	if(window == NULL)
	{
		fprintf(stderr, "Failed to open GLFW window.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) 
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

    glClearColor(0.4f, 0.4f, 0.4f, 0.0f);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	
	RLE rle;
	//printf("main probe 0\n");
	Ticker ticker;
	if (argc > 1)
	{
	    // in this case we input a string via the command line and render it to the universe
	    //TODO still won't work for symbols like ampersands due to file naming
	    //TODO verify correct index here. should have flags checking
	    ticker.createTickerFromString(argv[1]);
	    ticker.setPatternDataToRLE(&rle);
	}
    else
    {
        //std::string filename = "char_data/m.rle"; //TODO should be a command line flag entry for the filename and if we are to use a full file
        //rle.loadRLE(filename);
        //char chars[] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','!','?',',','"','\'','.',':',';','&','*','+','-', '<','=','>','@','$','#','_','[',']','{','}','|','\\','/','(',')','^','%',',','`',',','~',' '};
        //rle.loadCharArrayRLE(chars);
        //RLE tmpRLE;
        //tmpRLE.loadRLE(filename);
        //rle.loadRLE(filename);
        
        // construct from primative componenets
        uint stopperDistance = 2;
        uint headDistance = 0;
        uint beltLength = 0;
        uint numBeltGliders;
        uint numBelts = 4;
        bool * beltProgram;
        
        ticker.setStopperDistance(stopperDistance);
        ticker.setHeadDistance(headDistance);
        ticker.setBeltLength(beltLength);
        ticker.setNumBelts(numBelts);
        
        numBeltGliders = ticker.getNumBeltGliders();
        printf("numBeltGliders = %i\n", numBeltGliders); //TODO verbose mode
        beltProgram = new bool[numBeltGliders];
        for (uint i = 0; i < numBeltGliders; i++)
            beltProgram[i] = true;
        beltProgram[1] = false;
        
        for (uint i = 0; i < numBelts; i++)
            ticker.programBelt(i, beltProgram); // copies values
        delete[] beltProgram;
        
        ticker.setPatternDataToRLE(&rle);
        
    }
    
    GameOfLife gameOfLife(rle.x, rle.y, rle.data, true); //TODO could just pass in rle 

    Renderer renderer(
        gameOfLife.getWorldData(), 
        gameOfLife.getColorBuffer(), 
        gameOfLife.getColorData()
    ); //TODO could just pass in gameOfLife
    double lastTime = glfwGetTime();
	double currentTime;
	double delta;
	uint iteration = 0;
    do
	{
	    glClear(GL_COLOR_BUFFER_BIT);

        currentTime = glfwGetTime();
		delta = currentTime - lastTime;
		lastTime = currentTime; //TODO

        //printf("time = %f, iteration = %i\n", delta, iteration);

        gameOfLife.update();

		renderer.display();

		// Swap buffers
		glfwSwapBuffers(window);

        //std::this_thread::sleep_for(std::chrono::milliseconds(10)); // wait seconds

		glfwPollEvents();

        iteration++;
	} // Check if the ESC key was pressed or the window was closed
	while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		  glfwWindowShouldClose(window) == 0);

	glfwTerminate();

    return 0;
}
