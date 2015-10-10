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

#ifndef ALLOCATORPARAMS_H
#define ALLOCATORPARAMS_H

#include <string>
#include <iostream>

enum AllocatorType
{
	BnB,
	ILP
};

struct AllocatorParams
{
	AllocatorType allocatorType;
	std::string name;
	double timeout; // timeout in seconds
	int maxMigrationsRatio;

	// force class to be polymorphic
	virtual void dummy()
	{

	}
};

static AllocatorType stringToAllocatorType(const std::string& toConvert)
{
	if (toConvert == "BnB")
	{
		return BnB;
	}
	else if (toConvert == "ILP")
	{
		return ILP;
	}
	else
	{
		std::cout << "WARNING: Invalid Allocator Type. Defaulting to BnB." << std::endl;
		return BnB;
	}
}

#endif