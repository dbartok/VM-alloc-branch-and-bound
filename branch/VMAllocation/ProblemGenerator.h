/*
Copyright 2015 David Bartok, Zoltan Adam Mann

This file is part of VMAllocation.

VMAllocation is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

VMAllocation is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VMAllocation. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PROBLEMGENERATOR_H
#define PROBLEMGENERATOR_H

#include <string>

#include "AllocationProblem.h"


class ProblemGenerator
{
private:

	int dimension;
	int numVMs;
	int numPMs;
	int minResDemand;
	int maxResDemand;
	int minResSupply;
	int maxResSupply;
	int numPMTypes;

	static bool randomInitialized;
public:
	ProblemGenerator(int dimension, int nvms, int npms, int minrd, int maxrd, int minrs, int maxrs, int types);
	int randomIntBetween(int min, int max);
	AllocationProblem generate();
	AllocationProblem generate_ff();
	AllocationProblem testFromFile(std::string path);
};

#endif