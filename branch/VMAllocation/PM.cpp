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

#include "PM.h"

bool operator==(const PM& first, const PM& second)
{
	return first.id == second.id;
}

bool operator==(PM& first, PM& second)
{
	return first.id == second.id;
}

// all functions sort in some kind of ascending order

// lexicographic order
bool LexicographicPMComparator(PM* first, PM* second) 
{
	return first->resourcesFree < second->resourcesFree;
}

// maximum of resources
bool MaximumPMComparator(PM* first, PM* second)
{
	int firstMax  = 0;
	int secondMax = 0;
	for (size_t i = 0; i < first->resourcesFree.size(); i++)
	{
		if (first->resourcesFree[i] > firstMax)
			firstMax = first->resourcesFree[i];

		if (second->resourcesFree[i] > secondMax)
			secondMax = second->resourcesFree[i];
	}

	return firstMax < secondMax;
}

// sum of resources
bool SumPMComparator(PM* first, PM* second)
{
	int firstSum = 0;
	int secondSum = 0;
	for (size_t i = 0; i < first->resourcesFree.size(); i++)
	{
		firstSum += first->resourcesFree[i];
		secondSum += second->resourcesFree[i];
	}

	return firstSum < secondSum;
}

bool PM::isOn()
{
	for (size_t i = 0; i < resourcesFree.size(); i++)
	{
		if (resourcesFree[i] != capacity[i])
			return true;
	}

	return false;
}