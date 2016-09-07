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

#version 330 core

// Interpolated values from the vertex shaders
in vec2 particlecolor;

// Ouput data
out vec4 color;

void main()
{
	if (particlecolor.x == 0)
	{
	    color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        color = vec4(31.0f / 255.0f, 179.0f / 255.0f, 18.0f / 255.0f, 1.0f);
    }
}
