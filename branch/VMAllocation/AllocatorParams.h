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

#ifndef PARAMS_H
#define PARAMS_H

#include <vector>
#include <string>

enum SortType
{
	NONE,
	LEXICOPGRAPHIC,
	MAXIMUM,
	SUM
};

struct AllocatorParams
{
	std::string name;

	bool failFirst;

	SortType PMSortMethod;
	SortType VMSortMethod;
	bool initialPMFirst;
	bool symmetryBreaking; // causes the loss of optimality

	double timeout; // timeout in seconds
	double boundThreshold; // bound also when (cost >= bestSoFar * boundThreshold), makes sense when between 0 and 1

	int maxMigrations;
};

#endif