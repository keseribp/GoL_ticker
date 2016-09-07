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

#ifndef RLEFILE_H
#define RLEFILE_H

#include <string>
#include <string.h>

class RLE
{
    public:
        std::string hashtag; // should be #CXRLE on first line
        uint posX; // always present with no spaces on first line
        uint posY; // always present with no spaces on first line
        uint gen = 0; // optional on first line
        uint x; // always present on second line
        uint y; // always present on second line
        std::string rule; // always present on second line (GoL is B3/S23)
        bool * data;//[][]; // configuration of the cell array
    
    private:
        void _loadRLE(std::string filename, bool deletePrevData);
    
    public:
        RLE();
        ~RLE();
        void loadRLE(std::string filename);
        void loadCharArrayRLE(char * chars);
        void loadCharArrayRLE(std::string str);
};

#endif
