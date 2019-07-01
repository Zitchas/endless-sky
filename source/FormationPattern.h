/* FormationPattern.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FORMATION_PATTERN_H_
#define FORMATION_PATTERN_H_

#include "Angle.h"
#include "DataNode.h"
#include "Point.h"

#include <string>
#include <vector>

// Class representing a definition of a spaceship flying formation as loaded
// from datafiles or savegame when used in GameData.
class FormationPattern {
public:
	// Empty initializer
	FormationPattern();
	
	const std::string Name() const;
	
	// Load formation from datafile
	void Load(const DataNode &node);
	
	// Calculate next line and amount of positions on a line
	int NextLine(unsigned int iteration, unsigned int lineNr) const;
	int PositionsOnLine(unsigned int iteration, unsigned int lineNr) const;
	
	// Calculate a position based on an iteration number, line number and position-on-line number
	Point Position(unsigned int iteration, unsigned int lineNr, unsigned int posOnLine) const;

protected:
	class Line {
	public:
		Line(double X, double Y, int nrPositions, double directionAngle);
		
		// The initial anchor point for this line
		Point anchor;
		// Vector to apply to get to the next anchor point for the next iteration
		Point repeatVector;
		
		// The direction angle in which this line extends and space between any
		// 2 ship slots on the line. (Defaults to 2)
		Angle direction;
		double spacing = 2;
		
		// The number of initial positions for this line and the amount of additional
		// positions each iteration. slotsIncrease -1 is for lines that don't repeat.
		int initialSlots = 1;
		int slotsIncrease = -1;
	};
	
protected:
	// The line definitions that define the formation.
	std::vector<Line> lines;
	
private:
	std::string name;
};

#endif
