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

#ifndef TICKER_H
#define TICKER_H

#include <iostream>
#include <string>
#include <string.h>

#include "rleFile.h"

class Ticker
{
    private:
        RLE m_stopperRLE;
        RLE m_headRLE;
        RLE m_beltARLE;
        RLE m_beltBRLE;
        RLE m_gliderRLE;
    
        std::string m_tickerText;
        uint m_stopperDistance;
        uint m_headDistance;
        uint m_beltLength;
        uint m_numBeltGliders;
        uint m_numBelts;
        bool * m_beltPrograms; //an array of belt programs. col is a single belt. all belts are same length.
        bool * m_data; // the GoL pattern
    
        uint m_univX;
        uint m_univY;
        uint m_beltCompX;
        uint m_beltCompY;
    
        int c_headTransVect[2];
        int c_beltATransVect[2];
        int c_beltBTransVect[2];
        int c_stopperOriginVect[2];
        int c_headOriginVect[2];
        int c_beltAOriginVect[2];
        int c_beltBOriginVect[2];
        int c_gliderTrainTopOriginVect[2];
        int c_gliderTrainBotOriginVect[2];
        int c_loafTrainOriginVect[2];
        int c_beltAGliderOrigins[2 * 3];
        int c_beltAGliderOriginsSP[2 * 3]; //TODO do not need
        int c_beltBGliderOrigins[2 * 2];
        int c_beltBGliderOriginsSP[2 * 2]; //TODO do not need
    
        // translation vectors
        static const int m_headTransVect[2]; //TODO should load this in from the component file
        static const int m_beltATransVect[2];
        static const int m_beltBTransVect[2];
        static const int m_beltCompTransVect[2];
        static const int m_beltCompSpecialTransVect[2];
        // glider trains will have a special translation vector since they have a wobble to them

        // component origin vectors
        static const int m_stopperOriginVect[2]; //TODO should load this in from the component file
        static const int m_headOriginVect[2];
        static const int m_beltAOriginVect[2];
        static const int m_beltBOriginVect[2];
        static const int m_gliderTrainTopOriginVect[2];
        static const int m_gliderTrainBotOriginVect[2];
        static const int m_loafTrainOriginVect[2];
        
        // belt A glider locations
        static const int m_beltAGliderOrigins[2 * 3];
        static const int m_beltAGliderOriginsSP[2 * 3];
        
        // belt B glider locations
        static const int m_beltBGliderOrigins[2 * 2];
        static const int m_beltBGliderOriginsSP[2 * 2];
    
    private:
        void _initialize();
        void _reflectXAxis(uint x, uint y, bool * data);
        
        void _step(uint numSteps, uint x, uint y, bool * data);
        void _compileTicker();
        void _compileFullBelt(
            bool * patternData, 
            bool isReflected, 
            bool stopperIsReflected, 
            bool isAdvanced2Gens,
            uint numLoafs,
            uint beltIdx
        );
        void _compileStopper(
            uint tmpStopperX, 
            uint tmpStopperY, 
            bool * tmpStopperData, 
            bool stopperIsReflected
        );
        void _setAsFalse(
            bool * pattern,
            uint patX,
            uint patY,
            uint locX,
            uint locY,
            uint extX,
            uint extY
        );
        void _gol_update(uint x, uint y, bool * nextColor);
        void _gol_update_helper(
            uint index, 
            uint xIn, 
            uint yIn, 
            uint * prevColor, 
            bool * nextColor
        );
    
    public:
        Ticker();
        Ticker(std::string tickerText);
        ~Ticker();
        void genGlider(
            bool * glider, 
            std::string direction, 
            uint phase
        );
        void setGlider(
            bool * universe,
            uint univX,
            uint univY,
            bool glider[], 
            uint centerX, 
            uint centerY
        );
        void setStopper(
            bool * universe,
            uint univX,
            uint univY,
            bool glider[], 
            uint centerX, 
            uint centerY
        );
        void genLoaf(bool * loaf, std::string direction);
        void setLoaf(
            bool * universe,
            uint univX,
            uint univY, 
            bool loaf[],
            uint loafOX,
            uint loafOY
        );
        //void initialize(std::string compsDir); // pass the component directory //TODO later on this should be a .comp file that points to the various .rle with insertion points and other instructions
        void setStopperDistance(uint stopperDistance);
        void setHeadDistance(uint headDistance);
        void setBeltLength(uint beltLength);
        void setNumBelts(uint numBelts);
        uint getNumBeltGliders();
        void setPatternDataToRLE(RLE * rle);
        void programBelt(uint beltIdx, bool beltProgram[]);
        void createTickerFromString(std::string tickerText); // for now we only use the pixel princess text font
        void createTickerFromString(char * tickerText);
        uint getTickerPeriod();
};

#endif
