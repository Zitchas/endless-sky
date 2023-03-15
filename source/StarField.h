/* StarField.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef STAR_FIELD_H_
#define STAR_FIELD_H_

#include "Shader.h"

#include "opengl.h"

#include <vector>

class Body;
class Point;
class Sprite;
class System;



// Object to hold a set of "stars" to be drawn as a backdrop. The star pattern
// repeats every 4096 pixels. The pattern is generated by a random walk method
// so that some parts will be much denser than others, which is visually more
// interesting than if the stars were evenly spread out in perfectly random
// noise. If the view is moving, the stars are elongated in a motion blur to
// match the motion; otherwise they would seem to jitter around.
class StarField {
public:
	void Init(int stars, int width);
	void SetHaze(const Sprite *sprite, bool allowAnimation);

	void Draw(const Point &pos, const Point &vel, double zoom = 1., const System *system = nullptr, double fog = 0.) const;


private:
	void SetUpGraphics();
	void MakeStars(int stars, int width);


private:
	int widthMod;
	int tileCols;
	std::vector<int> tileIndex;

	// Track the haze sprite, so we can animate the transition between different hazes.
	const Sprite *lastSprite;
	mutable double transparency = 0.;
	std::vector<Body> haze[2];

	Shader shader;
	GLuint vao;
	GLuint vbo;

	GLuint offsetI;
	GLuint sizeI;
	GLuint cornerI;

	GLuint scaleI;
	GLuint rotateI;
	GLuint elongationI;
	GLuint translateI;
	GLuint brightnessI;

	GLint fogI;
	GLint zoomI;
};



#endif
