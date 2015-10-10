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

#ifndef ILPPARAMS_H
#define ILPPARAMS_H

#include <iostream>

#include "AllocatorParams.h"

enum SolverType
{
	LPSOLVE,
	GUROBI
};

struct ILPParams : public AllocatorParams
{
	SolverType solverType;
};

static SolverType stringToSolverType(const std::string& toConvert)
{
	if (toConvert == "LPSOLVE")
	{
		return LPSOLVE;
	}
	else if (toConvert == "GUROBI")
	{
		return GUROBI;
	}
	else
	{
		std::cout << "WARNING: Invalid Solver Type. Defaulting to LPSOLVE." << std::endl;
		return LPSOLVE;
	}
}

#endif