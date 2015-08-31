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

#include "VM.h"

// all functions sort in some kind of descending order

// lexicographic order
bool LexicographicVMComparator(const VM& first, const VM& second)
{
	return first.demand > second.demand;
}

// maximum of resources
bool MaximumVMComparator(const VM& first, const VM& second)
{
	int firstMax = 0;
	int secondMax = 0;
	for (std::size_t i = 0; i < first.demand.size(); i++)
	{
		if (first.demand[i] > firstMax)
			firstMax = first.demand[i];

		if (second.demand[i] > secondMax)
			secondMax = second.demand[i];
	}

	return firstMax > secondMax;
}

// sum of resources
bool SumVMComparator(const VM& first, const VM& second)
{
	int firstSum = 0;
	int secondSum = 0;
	for (std::size_t i = 0; i < first.demand.size(); i++)
	{
		firstSum += first.demand[i];
		secondSum += second.demand[i];
	}

	return firstSum > secondSum;
}
