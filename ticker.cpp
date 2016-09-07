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

#include "ticker.h"

#include "gameOfLife.h"

#include <algorithm>
#include <stdlib.h>

const int Ticker::m_headTransVect[2] = {46, 0}; //TODO should load this in from the component file
const int Ticker::m_beltATransVect[2] = {-23, -23};
const int Ticker::m_beltBTransVect[2] = {23, -23};
const int Ticker::m_beltCompTransVect[2] = {115, 18};
const int Ticker::m_beltCompSpecialTransVect[2] = {139, 18};
// glider trains will have a special translation vector since they have a wobble to them

// component origin vectors
const int Ticker::m_stopperOriginVect[2] = {0, 0}; //TODO should load this in from the component file
const int Ticker::m_headOriginVect[2] = {36, -24};
const int Ticker::m_beltAOriginVect[2] = {-42, -81};
const int Ticker::m_beltBOriginVect[2] = {-25, -77};
const int Ticker::m_gliderTrainTopOriginVect[2] = {-42 + 30, -81 + 11};
const int Ticker::m_gliderTrainBotOriginVect[2] = {-42 + 47, -81 + 32};
const int Ticker::m_loafTrainOriginVect[2] = {22, -13};

// belt A glider locations
const int Ticker::m_beltAGliderOrigins[2 * 3] = {
    -42 + 33, -81 + 43,
    -42 + 23, -81 + 35,
    -42 + 18, -81 + 20
};
const int Ticker::m_beltAGliderOriginsSP[2 * 3] = {
    -42 + 34, -81 + 43,
    -42 + 24, -81 + 35,
    -42 + 18, -81 + 21
};

// belt B glider locations
const int Ticker::m_beltBGliderOrigins[2 * 2] = {
    -25 + 14, -77 + 16,
    -25 + 26, -77 + 27
};
const int Ticker::m_beltBGliderOriginsSP[2 * 2] = {
    -25 + 14, -77 + 15,
    -25 + 25, -77 + 27
};

Ticker::Ticker()
{
    m_beltPrograms = nullptr;
    m_data = nullptr;
    _initialize();
}

Ticker::Ticker(std::string tickerText)
{
    m_beltPrograms = nullptr;
    m_data = nullptr;
    _initialize();
    
    // program ticker with string characters
    createTickerFromString(tickerText);
}

Ticker::~Ticker()
{
    //TODO implement
    //TODO belt needs to be deleted if not already done
}

//TODO need an option for if text is used to init
void Ticker::_initialize() //TODO this should be private and should be called on construction of the object
{
    // load components
    std::string compsDir = "components/";
    std::string stopperFilename = compsDir + "stopper.rle";
    std::string headFilename = compsDir + "head.rle";
    std::string beltAFilename = compsDir + "belt_A_111.rle";
    std::string beltBFilename = compsDir + "belt_B_11.rle";
    
    m_stopperRLE.loadRLE(stopperFilename);
    m_headRLE.loadRLE(headFilename);
    m_beltARLE.loadRLE(beltAFilename);
    m_beltBRLE.loadRLE(beltBFilename);
    
    // params
    m_stopperDistance = 0;//n_head;
    m_headDistance = 0;//n_belt_A;
    m_beltLength = 0;//n_belt_B;
    m_numBelts = 1;
    m_numBeltGliders = 5 + 4 * m_beltLength; // form is 5 + 4 * m_beltLength

    // belt data
    if (m_beltPrograms == nullptr) m_beltPrograms = new bool[m_numBeltGliders * m_numBelts]; // init as all false
    for (uint i = 0; i < m_numBeltGliders * m_numBelts; i++)
        m_beltPrograms[i] = true;
}

void Ticker::_compileTicker()
{
    uint buffer = 2 * 3;
    int specialBeltCompIdx;
    
    // determine which belts will be top and which will be bottom
    bool isReflected[m_numBelts];
    if (m_numBelts % 2 == 0)
    {
        for (uint i = 0; i < m_numBelts / 2; i++)
            isReflected[i] = false;
        for (uint i = m_numBelts / 2; i < m_numBelts; i++)
            isReflected[i] = true;
        specialBeltCompIdx = m_numBelts / 2;
    }
    else if (m_numBelts == 1)
    {
        isReflected[0] = false;
        specialBeltCompIdx = -1;
    }
    else
    {
        for (uint i = 0; i < (m_numBelts + 1) / 2; i++)
            isReflected[i] = false;
        for (uint i = (m_numBelts + 1) / 2; i < m_numBelts; i++)
            isReflected[i] = true;
        specialBeltCompIdx = (m_numBelts + 1) / 2;
    }
    
    uint numLoafs[m_numBelts];
    
    if (m_numBelts > 1)
    {
        for (uint i = 0; i < m_numBelts; i++) //TODO right now this is the max number. need to adjust based on glider programming
        {
            numLoafs[i] = 0;
            if (i == specialBeltCompIdx - 1)
            {
                numLoafs[i] = 1; // actually counting the downfacing loafs that delay only once
            }
            else if (i < specialBeltCompIdx)
            {
                numLoafs[i] = 1 + 5 * (specialBeltCompIdx - i - 1);
            }
            else if (i > specialBeltCompIdx)
            {
                numLoafs[i] = 1 + 5 * (i - specialBeltCompIdx);
            }
        }
    }
    else
    {
        numLoafs[0] = 0;
    }
    
    // need the compiler to set gliders based on belt programming
    // adjust loaf delays (and head distances) based on belt programming
    // use the maxLoafs to determine where our insertion point will be for a given belt
    bool tmpBeltPrograms[m_numBeltGliders * m_numBelts];
    uint maxNumLoafs = 0;
    uint absentGliderSum;
    
    for (uint j = 0; j < m_numBelts; j++)
    {
        for (uint i = 0; i < m_numBeltGliders; i++)
        {
            tmpBeltPrograms[
                m_numBelts * (static_cast<int>(i + numLoafs[j]) % m_numBeltGliders) + j
             ] = m_beltPrograms[m_numBelts * i + j];
        }
    }
    
    uint tmpNumLoafs;
    for (uint j = 0; j < m_numBelts; j++)
    {
        for (uint i = 0; i < m_numBeltGliders; i++)
            m_beltPrograms[m_numBelts * i + j] = tmpBeltPrograms[m_numBelts * i + j];
        
        absentGliderSum = 0;
        
        tmpNumLoafs = numLoafs[j];
        for (int i = 0; i < tmpNumLoafs; i++)
        {
            if (!m_beltPrograms[m_numBelts * (i % m_numBeltGliders) + j])
                absentGliderSum++;
        }
        
        numLoafs[j] = numLoafs[j] - absentGliderSum;
        maxNumLoafs = std::max(maxNumLoafs, numLoafs[j]);
    }
    
    m_headDistance = maxNumLoafs / 8;

    c_headTransVect[0] = m_stopperDistance * m_headTransVect[0];
    c_headTransVect[1] = m_stopperDistance * m_headTransVect[1];
    
    c_beltATransVect[0] = m_headDistance * m_beltATransVect[0];
    c_beltATransVect[1] = m_headDistance * m_beltATransVect[1];
    
    c_beltBTransVect[0] = m_beltLength * m_beltBTransVect[0];
    c_beltBTransVect[1] = m_beltLength * m_beltBTransVect[1];

    // modified component origin vectors
    c_stopperOriginVect[0] = m_stopperOriginVect[0];
    c_stopperOriginVect[1] = m_stopperOriginVect[1];
    
    c_headOriginVect[0] = m_headOriginVect[0] + c_headTransVect[0];
    c_headOriginVect[1] = m_headOriginVect[1] + c_headTransVect[1];
    
    c_beltAOriginVect[0] = m_beltAOriginVect[0] + c_headTransVect[0] + c_beltATransVect[0];
    c_beltAOriginVect[1] = m_beltAOriginVect[1] + c_headTransVect[1] + c_beltATransVect[1];
    
    c_beltBOriginVect[0] = m_beltBOriginVect[0] + c_headTransVect[0] 
                           + c_beltATransVect[0] + c_beltBTransVect[0];
    c_beltBOriginVect[1] = m_beltBOriginVect[1] + c_headTransVect[1] 
                           + c_beltATransVect[1] + c_beltBTransVect[1];

    c_gliderTrainTopOriginVect[0] = m_gliderTrainTopOriginVect[0] + c_headTransVect[0] 
                                    + c_beltATransVect[0];
    c_gliderTrainTopOriginVect[1] = m_gliderTrainTopOriginVect[1] + c_headTransVect[1] 
                                    + c_beltATransVect[1];
    
    c_gliderTrainBotOriginVect[0] = m_gliderTrainBotOriginVect[0] + c_headTransVect[0] 
                                    + c_beltATransVect[0];
    c_gliderTrainBotOriginVect[1] = m_gliderTrainBotOriginVect[1] + c_headTransVect[1] 
                                    + c_beltATransVect[1];

    c_loafTrainOriginVect[0] = m_loafTrainOriginVect[0] + c_headTransVect[0];
    c_loafTrainOriginVect[1] = m_loafTrainOriginVect[1] + c_headTransVect[1];

    c_beltAGliderOrigins[0] = m_beltAGliderOrigins[0] + c_headTransVect[0] 
                              + c_beltATransVect[0];
    c_beltAGliderOrigins[1] = m_beltAGliderOrigins[1] + c_headTransVect[1] 
                              + c_beltATransVect[1];
    c_beltAGliderOrigins[2] = m_beltAGliderOrigins[2] + c_headTransVect[0] 
                              + c_beltATransVect[0];
    c_beltAGliderOrigins[3] = m_beltAGliderOrigins[3] + c_headTransVect[1] 
                              + c_beltATransVect[1];
    c_beltAGliderOrigins[4] = m_beltAGliderOrigins[4] + c_headTransVect[0] 
                              + c_beltATransVect[0];
    c_beltAGliderOrigins[5] = m_beltAGliderOrigins[5] + c_headTransVect[1] 
                              + c_beltATransVect[1];
    
    c_beltAGliderOriginsSP[0] = m_beltAGliderOriginsSP[0] + c_headTransVect[0] 
                                + c_beltATransVect[0];
    c_beltAGliderOriginsSP[1] = m_beltAGliderOriginsSP[1] + c_headTransVect[1] 
                                + c_beltATransVect[1];
    c_beltAGliderOriginsSP[2] = m_beltAGliderOriginsSP[2] + c_headTransVect[0] 
                                + c_beltATransVect[0];
    c_beltAGliderOriginsSP[3] = m_beltAGliderOriginsSP[3] + c_headTransVect[1] 
                                + c_beltATransVect[1];
    c_beltAGliderOriginsSP[4] = m_beltAGliderOriginsSP[4] + c_headTransVect[0] 
                                + c_beltATransVect[0];
    c_beltAGliderOriginsSP[5] = m_beltAGliderOriginsSP[5] + c_headTransVect[1] 
                                + c_beltATransVect[1];
    
    c_beltBGliderOrigins[0] = m_beltBGliderOrigins[0] + c_headTransVect[0] 
                              + c_beltATransVect[0] + c_beltBTransVect[0];
    c_beltBGliderOrigins[1] = m_beltBGliderOrigins[1] + c_headTransVect[1] 
                              + c_beltATransVect[1] + c_beltBTransVect[1];
    c_beltBGliderOrigins[2] = m_beltBGliderOrigins[2] + c_headTransVect[0] 
                              + c_beltATransVect[0] + c_beltBTransVect[0];
    c_beltBGliderOrigins[3] = m_beltBGliderOrigins[3] + c_headTransVect[1] 
                              + c_beltATransVect[1] + c_beltBTransVect[1];
    
    c_beltBGliderOriginsSP[0] = m_beltBGliderOriginsSP[0] + c_headTransVect[0] 
                                + c_beltATransVect[0] + c_beltBTransVect[0];
    c_beltBGliderOriginsSP[1] = m_beltBGliderOriginsSP[1] + c_headTransVect[1] 
                                + c_beltATransVect[1] + c_beltBTransVect[1];
    c_beltBGliderOriginsSP[2] = m_beltBGliderOriginsSP[2] + c_headTransVect[0] 
                                + c_beltATransVect[0] + c_beltBTransVect[0];
    c_beltBGliderOriginsSP[3] = m_beltBGliderOriginsSP[3] + c_headTransVect[1] 
                                + c_beltATransVect[1] + c_beltBTransVect[1];

    // determine belt component size
    // this will be the max, in each direction x and y, of the origin plus the extents of
    //TODO are these change appropriately for the potential stopper reflections and placements?
    int xMax, xMin, yMax, yMin;
    
    xMin = std::min(
        c_stopperOriginVect[0],
        c_headOriginVect[0]
    );
    xMin = std::min(
        xMin,
        c_beltAOriginVect[0]
    );
    xMin = std::min(
        xMin,
        c_beltBOriginVect[0]
    );
    
    xMax = std::max(
        c_stopperOriginVect[0] + 2 * static_cast<int>(m_stopperRLE.x), // because of potential flips
        c_headOriginVect[0] + static_cast<int>(m_headRLE.x)
    );
    xMax = std::max(
        xMax,
        c_beltAOriginVect[0] + static_cast<int>(m_beltARLE.x)
    );
    xMax = std::max(
        xMax,
        c_beltBOriginVect[0] + static_cast<int>(m_beltBRLE.x)
    );
    m_beltCompX = xMax - xMin + buffer;
    printf("c_beltBOriginVect[0] + static_cast<int>(m_beltBRLE.x) = %i, c_beltBOriginVect[0] = %i, static_cast<int>(m_beltBRLE.x) = %i, m_beltBRLE.x = %i\n", c_beltBOriginVect[0] + static_cast<int>(m_beltBRLE.x), c_beltBOriginVect[0], static_cast<int>(m_beltBRLE.x), m_beltBRLE.x); //TODO verbose
    printf("m_beltCompX = %i, xMin = %i, xMax = %i\n", m_beltCompX, xMin, xMax); //TODO verbose
    
    yMin = std::min(
        c_stopperOriginVect[1],
        c_headOriginVect[1]
    );
    yMin = std::min(
        yMin,
        c_beltAOriginVect[1]
    );
    yMin = std::min(
        yMin,
        c_beltBOriginVect[1]
    );
    
    yMax = std::max(
        c_stopperOriginVect[1] + 2 * static_cast<int>(m_stopperRLE.y), // because of potential flips    
        c_headOriginVect[1] + static_cast<int>(m_headRLE.y)
    );
    yMax = std::max(
        yMax,
        c_beltAOriginVect[1] + static_cast<int>(m_beltARLE.y)
    );
    yMax = std::max(
        yMax,
        c_beltBOriginVect[1] + static_cast<int>(m_beltBRLE.y)
    );
    m_beltCompY = yMax - yMin + buffer;
    printf("m_beltCompY = %i, yMin = %i, yMax = %i\n", m_beltCompY, yMin, yMax); //TODO verbose

    c_stopperOriginVect[0] -= (xMin - buffer / 2);
    c_stopperOriginVect[1] -= (yMin - buffer / 2);
    
    c_headOriginVect[0] -= (xMin - buffer / 2);
    c_headOriginVect[1] -= (yMin - buffer / 2);
    
    c_beltAOriginVect[0] -= (xMin - buffer / 2);
    c_beltAOriginVect[1] -= (yMin - buffer / 2);
    
    c_beltBOriginVect[0] -= (xMin - buffer / 2);
    c_beltBOriginVect[1] -= (yMin - buffer / 2);
    
    c_gliderTrainTopOriginVect[0] -= (xMin - buffer / 2);
    c_gliderTrainTopOriginVect[1] -= (yMin - buffer / 2);
    
    c_gliderTrainBotOriginVect[0] -= (xMin - buffer / 2);
    c_gliderTrainBotOriginVect[1] -= (yMin - buffer / 2);
    
    c_loafTrainOriginVect[0] -= (xMin - buffer / 2);
    c_loafTrainOriginVect[1] -= (yMin - buffer / 2);
    
    c_beltAGliderOrigins[0] -= (xMin - buffer / 2);
    c_beltAGliderOrigins[1] -= (yMin - buffer / 2);
    c_beltAGliderOrigins[2] -= (xMin - buffer / 2);
    c_beltAGliderOrigins[3] -= (yMin - buffer / 2);
    c_beltAGliderOrigins[4] -= (xMin - buffer / 2);
    c_beltAGliderOrigins[5] -= (yMin - buffer / 2);
    
    c_beltBGliderOrigins[0] -= (xMin - buffer / 2);
    c_beltBGliderOrigins[1] -= (yMin - buffer / 2);
    c_beltBGliderOrigins[2] -= (xMin - buffer / 2);
    c_beltBGliderOrigins[3] -= (yMin - buffer / 2);
    
    // create a bool array to contain our temporary data as we create belt components and then later put them into the universe one by one
    bool tmpPatternData[m_beltCompX * m_beltCompY];

    // mark which bottom belt will be special (will be translated differently and set to +2 gens due to a need to avoid collisions with other full belt footprints)
    
    // determine belt component universe-relative origin vectors
    int beltCompOriginVects[2 * m_numBelts];
    //beltCompOriginVects[0] = -m_beltCompTransVect[0]; // so first += op is zero
    //beltCompOriginVects[1] = -m_beltCompTransVect[1]; // so first += op is zero
    int prevX = -m_beltCompTransVect[0]; // so first += op is zero
    int prevY = -m_beltCompTransVect[1]; // so first += op is zero
    
    // determine m_univX and m_univY
    m_univX = 0;
    m_univY = 0;
    int tmpUnivXMin = 0;
    int tmpUnivYMin = 0;
    int tmpUnivXMax = 0;
    int tmpUnivYMax = 0;
    
    // determine which belts will need to have flipped stoppers
    bool stopperIsReflected[m_numBelts];
    for (uint i = 0; i < m_numBelts; i++)
    {
        if (i % 2 == 0)
            stopperIsReflected[i] = false;    
        else
            stopperIsReflected[i] = true;
        
        if (isReflected[i])
        {
            if (i == specialBeltCompIdx)
            {
                printf("m_beltCompSpecialTransVect[0] = %i, m_beltCompTransVect[0] = %i\n", m_beltCompSpecialTransVect[0], m_beltCompTransVect[0]); //TODO verbose
                beltCompOriginVects[2 * i] = prevX + (m_beltCompSpecialTransVect[0] 
                                             - m_beltCompTransVect[0]);
                beltCompOriginVects[2 * i + 1] = prevY + m_beltCompY + 6; // 6 is the cell separation between the the two belts right at the reflection split
            }
            else if (i - 1 == specialBeltCompIdx)
            {
                beltCompOriginVects[2 * i] = prevX - ((m_beltCompSpecialTransVect[0] 
                                             - m_beltCompTransVect[0]) 
                                             + m_beltCompTransVect[0]);
                beltCompOriginVects[2 * i + 1] = prevY + m_beltCompTransVect[1];
            }
            else
            {
                beltCompOriginVects[2 * i] = prevX - m_beltCompTransVect[0];
                beltCompOriginVects[2 * i + 1] = prevY + m_beltCompTransVect[1];
            }
        }
        else
        {
            beltCompOriginVects[2 * i] = prevX + m_beltCompTransVect[0];
            beltCompOriginVects[2 * i + 1] = prevY + m_beltCompTransVect[1];
        }
        prevX = beltCompOriginVects[2 * i];
        prevY = beltCompOriginVects[2 * i + 1];
        
        printf("i = %i, beltCompOriginVects[2 * i] = %i, beltCompOriginVects[2 * i + 1] = %i\n", i, beltCompOriginVects[2 * i], beltCompOriginVects[2 * i + 1]); //TODO verbose
        printf("i = %i, beltCompOriginVects[2 * i] + static_cast<int>(m_beltCompX) = %i, beltCompOriginVects[2 * i + 1] + static_cast<int>(m_beltCompY) = %i\n", i, beltCompOriginVects[2 * i] + static_cast<int>(m_beltCompX), beltCompOriginVects[2 * i + 1] + static_cast<int>(m_beltCompY)); //TODO verbose
        tmpUnivXMin = std::min(tmpUnivXMin, beltCompOriginVects[2 * i]); 
        tmpUnivYMin = std::min(tmpUnivYMin, beltCompOriginVects[2 * i + 1]);
        tmpUnivXMax = std::max(tmpUnivXMax, beltCompOriginVects[2 * i] 
                      + static_cast<int>(m_beltCompX));
        tmpUnivYMax = std::max(tmpUnivYMax, beltCompOriginVects[2 * i + 1] 
                      + static_cast<int>(m_beltCompY));
    }
    m_univX = tmpUnivXMax - tmpUnivXMin + buffer;
    m_univY = tmpUnivYMax - tmpUnivYMin + buffer;
    printf("m_univX = %i, m_univY = %i\n", m_univX, m_univY);
    
    if (m_data == nullptr) m_data = new bool[m_univX * m_univY];
    for (uint i = 0; i < m_univX * m_univY; i++)
        m_data[i] = false; //TODO could probably be faster with a memcpy of all false
    uint tmpStopperX = 10;
    uint tmpStopperY = 10;
    bool tmpStopperData[tmpStopperX * tmpStopperY];
    // for each belt we need to compile
    for(uint k = 0; k < m_numBelts; k++)
    {
        // call _compileFullBelt with options passed about stopper orientation, gen, and overall orientation
        _compileFullBelt(
            tmpPatternData, 
            isReflected[k], 
            stopperIsReflected[k], 
            k == specialBeltCompIdx,
            numLoafs[k],
            k
        );
        
        // place stopper separately
        for (uint ii = 0; ii < tmpStopperX * tmpStopperY; ii++)
            tmpStopperData[ii] = false;
        _compileStopper(tmpStopperX, tmpStopperY, tmpStopperData, stopperIsReflected[k]);
        
        printf("beltCompOriginVects[2 * k] = %i, beltCompOriginVects[2 * k + 1] = %i\n", beltCompOriginVects[2 * k], beltCompOriginVects[2 * k + 1]); //TODO verbose
        
        // OR belt component into the universe
        //TODO could probably make this OR-injection into a general method
        if (isReflected[k])
        {
            for (uint i = 0; i < m_beltCompX; i++)
            {
                for (uint j = 0; j < m_beltCompY; j++)
                {            
                    m_data[m_univY * (beltCompOriginVects[2 * k] + i) 
                           + (beltCompOriginVects[2 * k + 1] + j - 12)
                    ] = m_data[m_univY * (beltCompOriginVects[2 * k] + i) 
                               + (beltCompOriginVects[2 * k + 1] + j - 12)
                    ] || tmpPatternData[m_beltCompY * i + j];
                }
            }
        }
        else
        {
            for (uint i = 0; i < m_beltCompX; i++)
            {
                for (uint j = 0; j < m_beltCompY; j++)
                {            
                    m_data[m_univY * (beltCompOriginVects[2 * k] + i) 
                           + (beltCompOriginVects[2 * k + 1] + j)
                    ] = m_data[m_univY * (beltCompOriginVects[2 * k] + i) 
                               + (beltCompOriginVects[2 * k + 1] + j)
                    ] || tmpPatternData[m_beltCompY * i + j];
                }
            }
        }
        
        if (isReflected[k])
        {
            if (stopperIsReflected[k])
            {
                for (uint i = 0; i < tmpStopperX; i++)
                {
                    for (uint j = 0; j < tmpStopperY; j++)
                    {
                        m_data[m_univY * (c_stopperOriginVect[0] + i) 
                               + (beltCompOriginVects[2 * k + 1] 
                               + c_stopperOriginVect[1] + j + 4 - m_beltCompY)
                        ] = m_data[m_univY * (c_stopperOriginVect[0] + i) 
                                   + (beltCompOriginVects[2 * k + 1] 
                                   + c_stopperOriginVect[1] + j + 4 - m_beltCompY)
                        ] || tmpStopperData[tmpStopperY * i + j];
                    }
                }
            }
            else
            {
                for (uint i = 0; i < tmpStopperX; i++)
                {
                    for (uint j = 0; j < tmpStopperY; j++)
                    {
                        m_data[m_univY * (c_stopperOriginVect[0] + i) 
                               + (beltCompOriginVects[2 * k + 1] 
                               + c_stopperOriginVect[1] + j + 9 - m_beltCompY)
                        ] = m_data[m_univY * (c_stopperOriginVect[0] + i) 
                                   + (beltCompOriginVects[2 * k + 1] 
                                   + c_stopperOriginVect[1] + j + 9 - m_beltCompY)
                        ] || tmpStopperData[tmpStopperY * i + j];
                    }
                }
            }
        }
        else
        {
            if (stopperIsReflected[k])
            {
                for (uint i = 0; i < tmpStopperX; i++)
                {
                    for (uint j = 0; j < tmpStopperY; j++)
                    {
                        m_data[m_univY * (c_stopperOriginVect[0] + i) 
                               + (beltCompOriginVects[2 * k + 1] 
                               + c_stopperOriginVect[1] + j - 9)
                        ] = m_data[m_univY * (c_stopperOriginVect[0] + i) 
                                   + (beltCompOriginVects[2 * k + 1] 
                                   + c_stopperOriginVect[1] + j - 9)
                        ] || tmpStopperData[tmpStopperY * i + j];
                    }
                }
            }
            else
            {
                for (uint i = 0; i < tmpStopperX; i++)
                {
                    for (uint j = 0; j < tmpStopperY; j++)
                    {
                        m_data[m_univY * (c_stopperOriginVect[0] + i) 
                               + (beltCompOriginVects[2 * k + 1] 
                               + c_stopperOriginVect[1] + j - 4)
                        ] = m_data[m_univY * (c_stopperOriginVect[0] + i) 
                                   + (beltCompOriginVects[2 * k + 1] 
                                   + c_stopperOriginVect[1] + j - 4)
                        ] || tmpStopperData[tmpStopperY * i + j];
                    }
                }
            }
        }
    }
}

void Ticker::_compileStopper(uint tmpStopperX, uint tmpStopperY, bool * tmpStopperData, bool stopperIsReflected)
{
    setStopper(
        tmpStopperData,
        tmpStopperX,
        tmpStopperY,
        m_stopperRLE.data, 
        m_stopperRLE.x, 
        m_stopperRLE.y
    );
 
    //TODO reflect the stopper, if necessary   
    if (stopperIsReflected) _reflectXAxis(tmpStopperX, tmpStopperY, tmpStopperData);
}

// patternData is an array output of length m_beltCompX * m_beltCompY
void Ticker::_compileFullBelt(bool * patternData, 
                              bool isReflected, 
                              bool stopperIsReflected, 
                              bool isAdvanced2Gens,
                              uint numLoafs,
                              uint beltIdx) //stopper reflection is relative to total reflection
{
    //TODO need to adjust glider origin vectors for the top and bot train
    // translation vectors
    
    if (patternData == nullptr) patternData = new bool[m_beltCompX * m_beltCompY];
    for (uint i = 0; i < m_beltCompX * m_beltCompY; i++)
        patternData[i] = false;
    
    // OR-insert head component
    for (uint i = 0; i < m_headRLE.x; i++)
    {
        for (uint j = 0; j < m_headRLE.y; j++)           
            patternData[m_beltCompY * (c_headOriginVect[0] + i) 
                        + (c_headOriginVect[1] + j)
            ] = patternData[m_beltCompY * (c_headOriginVect[0] + i) 
                            + (c_headOriginVect[1] + j)
            ] || m_headRLE.data[m_headRLE.y * i + j];
    } 
     
    // OR-insert belt_A component
    for (uint i = 0; i < m_beltARLE.x; i++)
    {
        for (uint j = 0; j < m_beltARLE.y; j++)           
            patternData[m_beltCompY * (c_beltAOriginVect[0] + i) 
                        + (c_beltAOriginVect[1] + j)
            ] = patternData[m_beltCompY * (c_beltAOriginVect[0] + i) 
                            + (c_beltAOriginVect[1] + j)
            ] || m_beltARLE.data[m_beltARLE.y * i + j];
    }
     
    // OR-insert belt_B componenet
    for (uint i = 0; i < m_beltBRLE.x; i++)
    {
        for (uint j = 0; j < m_beltBRLE.y; j++)           
            patternData[m_beltCompY * (c_beltBOriginVect[0] + i) 
                        + (c_beltBOriginVect[1] + j)
            ] = patternData[m_beltCompY * (c_beltBOriginVect[0] + i) 
                            + (c_beltBOriginVect[1] + j)
            ] || m_beltBRLE.data[m_beltBRLE.y * i + j];
    }
     
    // fill top belt glider slots
    bool glider[5 * 5];
    for (uint i = 0; i < 2 * m_beltLength; i++)
    {
        genGlider(glider, "SW", (1 + 2 * i) % 4); // glider, direction, phase
        setGlider(
            patternData,
            m_beltCompX,
            m_beltCompY, 
            glider, 
            c_gliderTrainTopOriginVect[0] + (11 * i + (i - i % 2) / 2), // leading corner coords
            c_gliderTrainTopOriginVect[1] - (11 * (i + 1) + ((i + 1) - (i + 1) % 2) / 2) + 11
        );
    }
    
    // fill bot belt glider slots
    for (uint i = 0; i < 2 * m_beltLength; i++)
    {
        genGlider(glider, "NE", (0 + 2 * i) % 4);
        setGlider(
            patternData,
            m_beltCompX,
            m_beltCompY, 
            glider,
            c_gliderTrainBotOriginVect[0] + (11 * i + (i - i % 2) / 2), // leading corner coords
            c_gliderTrainBotOriginVect[1] - (11 * (i + 1) + ((i + 1) - (i + 1) % 2) / 2) + 11
        );
    }
    
    //TODO remove gliders in belt based on belt program data
    printf("m_numBeltGliders = %i, m_beltLength = %i\n", m_numBeltGliders, m_beltLength); //TODO verbose
    uint j;
    for (uint i = 0; i < m_numBeltGliders; i++)
    {
        // n = 3 + 4*k + 2
        // k is belt length
        // n is num belt gliders
        if (!m_beltPrograms[m_numBelts * i + beltIdx])
        {
            printf("belt # = %i, removing glider # = %i\n", beltIdx, i); //TODO verbose
            // if i == 0, 4*k + 4, 4*k + 5 then belt A gliders
            if (i == 0)
            {
                printf("i == 0 case\n"); //TODO verbose
                _setAsFalse(
                    patternData,
                    m_beltCompX,
                    m_beltCompY, 
                    c_beltAGliderOrigins[4], 
                    c_beltAGliderOrigins[5], 
                    3, 
                    3
                );
            }
            else if (i == 4 * m_beltLength + 3)
            {
                printf("i == 4 * m_beltLength + 3 case\n"); //TODO verbose
                _setAsFalse(
                    patternData,
                    m_beltCompX,
                    m_beltCompY,
                    c_beltAGliderOrigins[0], 
                    c_beltAGliderOrigins[1], 
                    3, 
                    3
                );
            }
            else if (i == 4 * m_beltLength + 4)
            {
                printf("i == 4 * m_beltLength + 4 case\n"); //TODO verbose
                _setAsFalse(
                    patternData,
                    m_beltCompX,
                    m_beltCompY,
                    c_beltAGliderOrigins[2], 
                    c_beltAGliderOrigins[3], 
                    3, 
                    3
                );
            }
            // else if i == 2*k + 1, 2*k + 2 then belt B gliders
            else if (i == 2 * m_beltLength + 1)
            {
                printf("i == 2 * m_beltLength + 1 case\n"); //TODO verbose
                _setAsFalse(
                    patternData,
                    m_beltCompX,
                    m_beltCompY,
                    c_beltBGliderOrigins[0], 
                    c_beltBGliderOrigins[1], 
                    3, 
                    3
                );
            }
            else if (i == 2 * m_beltLength + 2)
            {
                printf("i == 2 * m_beltLength + 2 case\n"); //TODO verbose
                _setAsFalse(
                    patternData,
                    m_beltCompX,
                    m_beltCompY,
                    c_beltBGliderOrigins[2], 
                    c_beltBGliderOrigins[3], 
                    3, 
                    3
                );
            }
            // else if i == 1, ..., 2*k
            else if (i > 0 && i < 2 * m_beltLength + 1) // top belt gliders
            {
                printf("top belt glider case\n"); //TODO verbose
                j = i - 1;
                _setAsFalse(
                    patternData,
                    m_beltCompX,
                    m_beltCompY,
                    c_gliderTrainTopOriginVect[0] + (11 * j + (j - j % 2) / 2),
                    c_gliderTrainTopOriginVect[1] - (11 * (j + 1) + ((j + 1) - (j + 1) % 2) / 2) + 11 - 2,
                    3,
                    3
                );
            }    
            // else if i == 2*k + 3, ..., 4*k + 2
            else // bot belt gliders
            {
                printf("bot belt glider case\n"); //TODO verbose
                j = (2 * m_beltLength - 1) - (i - (2 * m_beltLength + 3));
                _setAsFalse(
                    patternData,
                    m_beltCompX,
                    m_beltCompY,
                    c_gliderTrainBotOriginVect[0] + (11 * j + (j - j % 2) / 2) - 2,
                    c_gliderTrainBotOriginVect[1] - (11 * (j + 1) 
                                                     + ((j + 1) - (j + 1) % 2) / 2) + 11,
                    3,
                    3
                );
            }
        }
    }
    
    // fill loaf delay slots
    bool loaf[4 * 4];
    
    for (uint i = 0; i < numLoafs / 2; i++)
    {
        genLoaf(loaf, "NW");
            
        setLoaf(
            patternData,
            m_beltCompX,
            m_beltCompY, 
            loaf,
            c_loafTrainOriginVect[0] - 5 * i, // leading corner coords
            c_loafTrainOriginVect[1] - 5 * i
        );
    }
    if (numLoafs % 2 == 1)
    {        
        genLoaf(loaf, "SE");
        setLoaf(
            patternData,
            m_beltCompX,
            m_beltCompY, 
            loaf,
            c_loafTrainOriginVect[0] - 5 * (numLoafs / 2), // leading corner coords
            c_loafTrainOriginVect[1] - 5 * (numLoafs / 2)
        );
    }
    
    // x-reflect entire component, if necessary
    if (isReflected) _reflectXAxis(m_beltCompX, m_beltCompY, patternData);
    
    // set component +2 generations, if necessary
    if (isAdvanced2Gens) _step(2, m_beltCompX, m_beltCompY, patternData);
}

void Ticker::setStopperDistance(uint stopperDistance) //TODO might not be the worst idea to allow for the user to make modifications and then hold off compilation until they are all done. currently we are recompiling the ticker after every change. might jsut do a lazy compilation where the thing is not constructed until it is asked for.
{
    m_stopperDistance = stopperDistance;
}

void Ticker::setHeadDistance(uint headDistance)
{
    m_headDistance = headDistance;
}

void Ticker::setBeltLength(uint beltLength)
{
    m_beltLength = beltLength;
    m_numBeltGliders = 5 + 4 * m_beltLength;
    if (m_beltPrograms != nullptr) delete[] m_beltPrograms;
    m_beltPrograms = new bool[m_numBeltGliders * m_numBelts];
}

void Ticker::setNumBelts(uint numBelts)
{
    m_numBelts = numBelts;
    if (m_beltPrograms != nullptr) delete[] m_beltPrograms;
    m_beltPrograms = new bool[m_numBeltGliders * m_numBelts];
}

uint Ticker::getNumBeltGliders()
{
    return m_numBeltGliders;
}

uint Ticker::getTickerPeriod()
{
    return 46 * m_numBeltGliders;
}

void Ticker::_setAsFalse(bool * pattern,
                         uint patX,
                         uint patY,
                         uint locX,
                         uint locY,
                         uint extX,
                         uint extY)
{
    for (uint i = 0; i < extX; i++)
    {
        for (uint j = 0; j < extY; j++)
            pattern[patY * (locX + i) + (locY + j)] = false;
    }
}

void Ticker::setPatternDataToRLE(RLE * rle)
{   
    // rle is output
    _compileTicker();
    rle->x = m_univX;
    rle->y = m_univY;
    rle->data = m_data;
}

void Ticker::programBelt(uint beltIdx, bool beltProgram[]) // copy values
{
    for (uint i = 0; i < m_numBeltGliders; i++)
        m_beltPrograms[m_numBelts * i + beltIdx] = beltProgram[i];
}

void Ticker::createTickerFromString(std::string tickerText) // for now we only use the pixel princess text font
{
    //TODO set all parameters and program the belt using the translation of the string chars to pixel princess chars held in .rle files
    //TODO do not compile here. we are going with a lazy compilation scheme instead.
    
    RLE stringRLE;
    stringRLE.loadCharArrayRLE(tickerText);
    
    // with our font we automatically choose the number of belts
    m_stopperDistance = 2 + 2 * stringRLE.x;
    m_beltLength = std::max(stringRLE.x - 1, static_cast<uint>(0)); // work shown in notepad
    m_numBelts = 2 * stringRLE.y;
    m_numBeltGliders = 5 + 4 * m_beltLength; // form is 5 + 4 * m_beltLength
    m_headDistance = 0; // likely to be readjusted

    // belt data
    if (m_beltPrograms != nullptr) delete[] m_beltPrograms;
    m_beltPrograms = new bool[m_numBeltGliders * m_numBelts]; // init as all false
    
    // for each string pixel
    for (uint i = 0; i < stringRLE.x; i++)
    {    
        // get row data and append to belt program
        for (uint j = 0; j < stringRLE.y; j++)
            m_beltPrograms[stringRLE.y * i + j] = stringRLE.data[stringRLE.y * i + j];
    }
    
    // everything else happens later due to lazy compilation scheme
}

void Ticker::createTickerFromString(char * tickerText) // for now we only use the pixel princess text font
{
    //TODO set all parameters and program the belt using the translation of the string chars to pixel princess chars held in .rle files
    //TODO do not compile here. we are going with a lazy compilation scheme instead.
    RLE stringRLE;
    stringRLE.loadCharArrayRLE(tickerText);
    
    // with our font we automatically choose the number of belts
    //m_stopperDistance = 2 + 2 * stringRLE.x;
    
    m_stopperDistance = 25; //TODO should really be adjustable
    
    m_beltLength = std::max(stringRLE.x / 2 - 1, static_cast<uint>(0)); // work shown in notepad
    m_numBelts = 2 * stringRLE.y;
    m_numBeltGliders = 5 + 4 * m_beltLength; // form is 5 + 4 * m_beltLength
    m_headDistance = 0; // likely to be readjusted
    
    printf("m_stopperDistance = %i\n", m_stopperDistance);
    printf("m_beltLength = %i\n", m_beltLength);
    printf("m_numBelts = %i\n", m_numBelts);
    printf("m_numBeltGliders = %i\n", m_numBeltGliders);
    printf("m_headDistance = %i\n", m_headDistance);

    // belt data
    if (m_beltPrograms != nullptr) delete[] m_beltPrograms;
    m_beltPrograms = new bool[m_numBeltGliders * m_numBelts]; // init as all false
    for (uint i = 0; i < m_numBeltGliders * m_numBelts; i++)
        m_beltPrograms[i] = false;
    
    // for each string pixel
    uint tmp = 0;
    for (uint i = 0; i < stringRLE.x; i++) // m_numBeltGliders
    {    
        // get row data and append to belt program
        for (uint j = 0; j < stringRLE.y; j++) // m_numBelts
        {
            if (m_numBelts * (2 * i + 1) + 2 * j + 1 < m_numBelts * m_numBeltGliders)
            {
                m_beltPrograms[m_numBelts * (2 * i) + 2 * j] 
                    = stringRLE.data[stringRLE.y * i + j];
                m_beltPrograms[m_numBelts * (2 * i) + 2 * j + 1] 
                    = stringRLE.data[stringRLE.y * i + j];
                m_beltPrograms[m_numBelts * (2 * i + 1) + 2 * j] 
                    = stringRLE.data[stringRLE.y * i + j];
                m_beltPrograms[m_numBelts * (2 * i + 1) + 2 * j + 1] 
                    = stringRLE.data[stringRLE.y * i + j];
            }
            tmp = std::max(tmp, m_numBelts * (2 * i + 1) + 2 * j + 1);
        }
    }
    printf("length of m_beltPrograms = %i\n", m_numBelts * m_numBeltGliders);  //TODO verbose
    printf("max idx = %i\n", tmp);  //TODO verbose
    
    for (uint j = 0; j < m_numBelts; j++)
    {
        for (uint i = 0; i < m_numBeltGliders; i++)
        {    
        // get row data and append to belt program
        
            if (m_beltPrograms[m_numBelts * i + j])
                printf("@"); //TODO verbose
            else
                printf("O"); //TODO verbose
        }
        printf("\n");
    }
    
    // everything else happens later due to lazy compilation scheme
}

//TODO this needs to be implemented to be more efficient
void Ticker::_reflectXAxis(uint x, uint y, bool * data)
{
    bool swapVal;
    bool newData [x * y];
    
    for (uint i = 0; i < x; i++) //TODO could do an initial check to make sure that evenHalfY > 0
    {
        for (uint j = 0; j < y; j++)
            newData[y * i + j] = data[y * (i + 1) - 1 - j];
    }
    
    for (uint i = 0; i < x; i++) //TODO could do an initial check to make sure that evenHalfY > 0
    {
        for (uint j = 0; j < y; j++)
        {
            data[y * i + j] = newData[y * i + j];
        }
    }
}

//TODO for now we assume that the rule is always GoL and that the RLE components we are using are not too large to just run via loop
//void Ticker::_stepRLE(uint numSteps, RLE * componentRLE)
void Ticker::_step(uint numSteps, uint x, uint y, bool * data)
{
    if (false)
    {
        GameOfLife gameOfLife(x, y, data);
        for (uint i = 0; i < numSteps; i++)
        {
            gameOfLife.update();
        }
        
        RLE * componentRLE = new RLE;
        componentRLE->x = x;
        componentRLE->y = y;
        gameOfLife.retrieveRLEData(componentRLE); // sets to the RLE
        data = componentRLE->data; //TODO may be leaking memory previously pointed to by data here
        delete componentRLE;
    }
    else
    {
        for (uint i = 0; i < numSteps; i++)
            _gol_update(x, y, data);
    }
}

// gliders are 5 x 5 bool
// fill glider and return
void Ticker::genGlider(bool * glider, std::string direction, uint phase) //TODO want to replace this by a glider loader instead. still want to load to a 3x3 cell array
{
    // set to all false
    for (uint i = 0; i < 5 * 5; i++)
        glider[i] = false;
    
    if (direction == "NE")
    {
        if (phase == 0)
        {
            glider[2] = true;
            glider[7] = true;
            glider[9] = true;
            glider[12] = true;
            glider[13] = true;
        }
        else if (phase == 1)
        {
            glider[4] = true;
            glider[7] = true;
            glider[8] = true;
            glider[13] = true;
            glider[14] = true;
        }
        else if (phase == 2)
        {
            glider[3] = true;
            glider[7] = true;
            glider[12] = true;
            glider[13] = true;
            glider[14] = true;
        }
        else if (phase == 3)
        {
            glider[2] = true;
            glider[7] = true;
            glider[8] = true;
            glider[4] = true;
            glider[13] = true;
        }
        else
        {
            //TODO error
        }
    }
    else if (direction == "SW")
    {
        if (phase == 0)
        {
            glider[11] = true;
            glider[12] = true;
            glider[15] = true;
            glider[17] = true;
            glider[22] = true;
        }
        else if (phase == 1)
        {
            glider[10] = true;
            glider[11] = true;
            glider[16] = true;
            glider[17] = true;
            glider[20] = true;
        }
        else if (phase == 2)
        {
            glider[10] = true;
            glider[11] = true;
            glider[12] = true;
            glider[17] = true;
            glider[21] = true;
        }
        else if (phase == 3)
        {
            glider[11] = true;
            glider[20] = true;
            glider[16] = true;
            glider[17] = true;
            glider[22] = true;
        }
        else
        {
            //TODO error
        }
    }
    else if (direction == "NW")
    {
        //TODO implement
    }
    else if (direction == "SE")
    {
        //TODO implement
    }
    else
    {
        //TODO error
    }
}

// glider is 5 x 5 bool
void Ticker::setGlider(bool * universe,
               uint univX,
               uint univY,
               bool glider[], 
               uint centerX, 
               uint centerY)
{
    printf("centerX = %i, centerY = %i\n", centerX, centerY); //TODO verbose
    for (uint i = 0; i < 5; i++)
    {
        for (uint j = 0; j < 5; j++)
        {
            universe[univY * (centerX + i - 2) + (centerY + j - 2)] 
                = universe[univY * (centerX + i - 2) + (centerY + j - 2)] 
                  || glider[5 * i + j];
        }
    }
}

// stopper is 10 x 10 bool
void Ticker::setStopper(bool * container,
               uint x,
               uint y,
               bool stopper[], 
               uint centerX, 
               uint centerY)
{
    for (uint i = 0; i < 4; i++)
    {
        for (uint j = 0; j < 4; j++)
            container[y * (0 + i) + (4 + j)] = stopper[4 * i + j];
    }
}

// loafs are 4 x 4 bool
void Ticker::genLoaf(bool * loaf, std::string direction)
{
    // set to all false
    for (uint i = 0; i < 4 * 4; i++)
        loaf[i] = false;
    
    if (direction == "NW")
    {
        loaf[1] = true;
        loaf[2] = true;
        loaf[4] = true;
        loaf[7] = true;
        loaf[8] = true;
        loaf[10] = true;
        loaf[13] = true;
    }
    else if (direction == "SE")
    {
        loaf[2] = true;
        loaf[5] = true;
        loaf[7] = true;
        loaf[8] = true;
        loaf[11] = true;
        loaf[13] = true;
        loaf[14] = true;
    }
    else if (direction == "NE")
    {
        //TODO implement
    }
    else if (direction == "SW")
    {
        //TODO implement
    }
    else
    {
        //TODO error
    }
}

void Ticker::setLoaf(bool * universe,
                       uint univX,
                       uint univY, 
                       bool loaf[],
                       uint loafOX,
                       uint loafOY)
{
    for (uint i = 0; i < 4; i++)
    {
        for (uint j = 0; j < 4; j++)
        {
            universe[univY * (loafOX + i) + (loafOY + j)] 
                = universe[univY * (loafOX + i) + (loafOY + j)] 
                  || loaf[4 * i + j];
        }
    }
}

/////////////////////////////////

//TODO cheating to get the +2 gen in. gameOfLife.cpp procedure won't work so just to get past this point we are looping here instead
void Ticker::_gol_update(uint x, uint y, bool * nextColor) 
{
    uint prevColor[x * y];
    
    for (uint i = 0; i < x; i++)
        for (uint j = 0; j < y; j++)
        {
            if (nextColor[y * i + j])
                prevColor[y * i + j] = 1;
            else
                prevColor[y * i + j] = 0;
        }
    for (uint i = 0; i < x; i++)
        for (uint j = 0; j < y; j++)
            _gol_update_helper(y * i + j, x, y, prevColor, nextColor);
}

void Ticker::_gol_update_helper(uint index, uint xIn, uint yIn, uint * prevColor, bool * nextColor) 
{
    uint width = xIn;
    uint height = yIn;
    uint numCells = width * height;
    
    uint x = index / height;
    uint y = index % height;
    uint x_m1 = (x - 1) % width;
    if (x - 1 == -1) x_m1 = (width - 1); // for correct modulus
    uint x_p1 = (x + 1) % width;
    uint y_m1 = (y - 1) % height;
    if (y - 1 == -1) y_m1 = (height - 1); // for correct modulus
    uint y_p1 = (y + 1) % height;
    bool s = (1 == prevColor[index]); // current state //TODO make sure that we have only 1 and 0 values for prevColor
    uint n = prevColor[((height * x_m1 + y_m1)) % (numCells)] //TODO redundant calculations here
             + prevColor[((height * x + y_m1)) % (numCells)]
             + prevColor[((height * x_p1 + y_m1)) % (numCells)]
             + prevColor[((height * x_m1 + y)) % (numCells)]
             //+ prevColor[(index + x + y)]
             + prevColor[((height * x_p1 + y)) % (numCells)]
             + prevColor[((height * x_m1 + y_p1)) % (numCells)]
             + prevColor[((height * x + y_p1)) % (numCells)]
             + prevColor[((height * x_p1 + y_p1)) % (numCells)];
    
    if (s && n < 2)
    {
        nextColor[index] = false;// dead
    }
    else if (s && (n == 2 || n == 3))
    {
        nextColor[index] = 1;// alive
    }
    else if (s && n > 3)
    {
        nextColor[index] = 0;// dead
    }
    else if (!s && n == 3)
    {
        nextColor[index] = 1;// alive
    }
    else
    {
        nextColor[index] = false; // previous
    }
}
