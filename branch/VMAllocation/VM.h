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

#ifndef VM_H
#define VM_H

#include <vector>

#include "PM.h"

struct VM
{
	int id;
	std::vector<int> demand;
	int initial; // ID of initially assigned PM
	std::vector<PM*>::iterator PMIterator; // "index" in the availablePMs array
	std::vector<PM*> availablePMs;
};

bool VMComparator(const VM& first, const VM& second);

bool LexicographicVMComparator(const VM& first, const VM& second);

bool MaximumVMComparator(const VM& first, const VM& second);

bool SumVMComparator(const VM& first, const VM& second);


#endif