/* BatchShader.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef BATCH_SHADER_H_
#define BATCH_SHADER_H_

class Sprite;

#include <vector>



// Class for drawing sprites in a batch. The input to each draw command is a
// sprite, whether it should be drawn high DPI, and the vertex data.
class BatchShader {
public:
	// Initialize the shaders.
	static void Init();

	static void Bind();
	static void Add(const Sprite *sprite, bool isHighDPI, const std::vector<float> &data, double fog = 0.);
	static void Unbind();
};



#endif
