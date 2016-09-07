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

#include "rleFile.h"

#include <iostream>
#include <fstream>
#include <regex>

void RLE::_loadRLE(std::string filename, bool deletePrevData)
{
    if (deletePrevData)
    {
        if (data != nullptr) delete[] data;
    }
    loadRLE(filename);
}
    
RLE::RLE()
{
    hashtag = ""; // should be #CXRLE on first line
    posX = 0; // always present with no spaces on first line
    posY = 0; // always present with no spaces on first line
    gen = 0; // optional on first line
    x = 0; // always present on second line
    y = 0; // always present on second line
    rule = ""; // always present on second line (GoL is B3/S23)
    data = nullptr;
};
        
RLE::~RLE()
{
    if (data != nullptr) delete[] data; //TODO this is dangerous because we don't check if the data was input via a locally allocated block of data instead of via new keyword
};
        
void RLE::loadRLE(std::string filename) //TODO need to do proper checks and throw useful errors
{
    hashtag = ""; // should be #CXRLE on first line
    posX = 0; // always present with no spaces on first line
    posY = 0; // always present with no spaces on first line
    gen = 0; // optional on first line
    x = 0; // always present on second line
    y = 0; // always present on second line
    rule = ""; // always present on second line (GoL is B3/S23)
    data = nullptr;

    std::string line;
    std::ifstream file;
    file.open(filename);
    if (file.is_open())
    {
        std::regex firstLineA("#([a-zA-Z]+) Pos=([0-9-]+),([0-9-]+) Gen=([0-9]+)");
        std::regex firstLineB("#([a-zA-Z]+) Pos=([0-9-]+),([0-9-]+)");
        std::regex secondLine("x = ([0-9]+), y = ([0-9]+), rule = ([a-zA-Z0-9/]+)");
     	
     	std::regex dataLine("([0-9]*)([ob$!])([0-9ob$!]*)");
        
        std::smatch matchFirstLine; //TODO may be able to just reuse 1 of these
        std::smatch matchSecondLine;
        std::smatch matchDataLine;
        
        std::getline(file, line);         
        
        std::regex_match(line, matchFirstLine, firstLineB);
        if (matchFirstLine.size() == 0)
        {
            std::regex_match(line, matchFirstLine, firstLineA);
            gen = abs(std::stoi(matchFirstLine[4].str()));
        } //TODO catch for incorrect file type
        else
        {
            gen = 0;
        }
        std::cout << line << std::endl;
        
        hashtag = matchFirstLine[1].str();
        std::cout << matchFirstLine[2].str() << std::endl;
        std::cout << std::stoi(matchFirstLine[2].str()) << std::endl;
        posX = abs(std::stoi(matchFirstLine[2].str()));
        posY = abs(std::stoi(matchFirstLine[3].str()));
        
        std::getline(file, line);
        
        std::regex_match(line, matchSecondLine, secondLine);
        
        x = abs(std::stoi(matchSecondLine[1].str()));
        y = abs(std::stoi(matchSecondLine[2].str()));
        rule = matchSecondLine[3].str(); // TODO should check this
        
        // create the data
        data = new bool[x * y];
        uint idx = 0;
        uint n = 0;
        bool mark = false;
        std::string delim;
        while (std::getline(file, line)) // specific state of the cells
        {
            std::cout << line << std::endl;
            while (line.size() > 0)
            {
                std::regex_match(line, matchDataLine, dataLine);
                
                delim = matchDataLine[2].str(); // delim indicator
                
                if (matchDataLine[1].str() == "")
                    n = 1;
                else
                    n = abs(std::stoi(matchDataLine[1].str()));
                
                if (delim == "b")
                {
                    mark = false;
                }
                else if (delim == "o")
                {
                    mark = true;
                }
                else if (delim == "$")
                {
                    mark = false;
                    n = (x - (idx % x)) + x * (n - 1);
                }
                else if (delim == "!")
                {
                    mark = false;
                    n = (x * y) - idx; // go until end of data array
                }
                else // else error
                {
                    printf("Error parsing file.\n");
                    return;
                }
                
                if (idx % x == 0 && 
                    idx != 0 &&
                    delim == "$" &&
                    n >= x)
                {
                    if(n > 1) 
                    {
                        n -= x;
                        for (uint i = 0; i < n; i++)
                            data[idx + i] = mark;
                        idx += n;
                    }
                    // otherwise do nothing
                }
                else
                {
                    for (uint i = 0; i < n; i++)
                        data[idx + i] = mark;
                    idx += n;
                }
                line = matchDataLine[3].str(); // rest of the string, if any 
            }
        }
        file.close();
    }
    else
    {
        printf("Failed to open %s.\n", filename.c_str());
    }
    
    // transpose data
    bool tmpData[x * y];
    for (uint i = 0; i < x * y; i++)
        tmpData[i] = data[i];
    for (uint i = 0; i < x; i++)
    {
        for (uint j = 0; j < y; j++)
        {
            data[i * y + j] = tmpData[j * x + i];
        }
    }
}

void RLE::loadCharArrayRLE(std::string str)
{
    char * writable = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), writable);
    writable[str.size()] = '\0'; // don't forget the terminating 0

    // don't forget to free the string after finished using it
    delete[] writable;   
    
    loadCharArrayRLE(writable);
}
        
void RLE::loadCharArrayRLE(char * chars)
{
    std::string dirName = "char_data/"; //TODO should we maybe start using a database for things like this?
    uint numChars = strlen(chars);
    //bool * tmpData;
    const uint pxY = 8; //TODO for now our font is 8 px high with variable width
    uint aggX = numChars; // all spaces between chars
    uint xHist[numChars];
    bool * aggData[numChars];
    std::string tmpChar;
    for (uint i = 0; i < numChars; i++) //for (char c : chars)
    {
        switch (chars[i])
        {
            case '!': tmpChar = "exclamation"; break;
            case '?': tmpChar = "question"; break;
            case ',': tmpChar = "comma"; break;
            case '"': tmpChar = "quotes"; break;
            case '\'': tmpChar = "apostrophe"; break;
            case '.': tmpChar = "period"; break;
            case ':': tmpChar = "colon"; break;
            case ';': tmpChar = "semicolon"; break;
            case '&': tmpChar = "ampersand"; break;
            case '*': tmpChar = "asterisk"; break;
            case '+': tmpChar = "plus"; break;
            case '-': tmpChar = "minus"; break;
            case '<': tmpChar = "lessthan"; break;
            case '=': tmpChar = "equals"; break;
            case '>': tmpChar = "greaterthan"; break;
            case '@': tmpChar = "at"; break;
            case '$': tmpChar = "dollar"; break;
            case '#': tmpChar = "pound"; break;
            case '_': tmpChar = "underscore"; break;
            case '[': tmpChar = "leftsquarebracket"; break;
            case ']': tmpChar = "rightsquarebracket"; break;
            case '{': tmpChar = "leftcurlybracket"; break;
            case '}': tmpChar = "rightcurlybracket"; break;
            case '|': tmpChar = "bar"; break;
            case '\\': tmpChar = "backslash"; break;
            case '/': tmpChar = "forwardslash"; break;
            case '(': tmpChar = "leftparenthese"; break;
            case ')': tmpChar = "rightparenthese"; break;
            case '^': tmpChar = "carat"; break;
            case '%': tmpChar = "percent"; break;
            case '`': tmpChar = "mark"; break;
            case '~': tmpChar = "congruent"; break;
            case ' ': tmpChar = "space"; break;
            default: tmpChar = std::string(1, chars[i]);
        } // TODO it might be better to do some sort of hash and not load letters more than once
        /*
        hashtag = ""; // should be #CXRLE on first line
        posX = 0; // always present with no spaces on first line
        posY = 0; // always present with no spaces on first line
        gen = 0; // optional on first line
        x = 0; // always present on second line
        y = 0; // always present on second line
        rule = ""; // always present on second line (GoL is B3/S23)
        data = nullptr;
        */
        _loadRLE(dirName + tmpChar + ".rle", false); //TODO likely needs to be a string. char_data dir needs to be some const too
        
        // append loaded data
        //for (uint j = 0; j < pxY; j++)
        //aggData.push_back(data); // data was allocated dynamically so it should not die at the end of this
        aggData[i] = data;
        //data = nullptr;  
            
        // aggregate y
        xHist[i] = x;
        aggX += x;
    }
    // set aggData to data with gaps between chars. set all other RLE data too
    x = aggX;
    y = pxY; 
    data = new bool[x * y]; //TODO should make sure the other data is set correctly too...
    bool * d;
    uint p = 0;
    for (uint c = 0; c < numChars; c++)
    {
        d = aggData[c];
        for (uint j = 0; j < pxY; j++)
        {
            for (uint i = 0; i < xHist[c]; i++)
            {
                data[(i + p) * y + j] = d[i * pxY + j]; // c-th char data
            }
        }
        p += xHist[c];
        for (uint j = 0; j < pxY; j++)
        {
            data[p * y + j] = false; // c-th char space
        }
        p += 1;
        delete[] d;
        d = nullptr;
    } //TODO do a calculation to verify no overflows
}
