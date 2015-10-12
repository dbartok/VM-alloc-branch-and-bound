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

#ifndef ILPALLOCATOR_H
#define ILPALLOCATOR_H

#include <fstream>
#include <memory>

#include "VMAllocator.h"
#include "AllocationProblem.h"
#include "AllocatorParams.h"
#include "ILPParams.h"

#define LPSOLVEPATH "lp_solve_5.5.2.0_exe\\lp_solve"

class ILPAllocator : public VMAllocator
{
	AllocationProblem m_problem; // the allocation problem
	ILPParams m_params; // algorithm parameters
	std::ofstream& m_log; // output log file
	SolverType m_solverType;

	int m_dimension; // dimension of resources
	int m_numVMs; // number of Virtual Machines
	int m_numPMs; // number of Physical Machines

	void create_lp(char *filename);

public:
	ILPAllocator(AllocationProblem pr, std::shared_ptr<AllocatorParams> pa, std::ofstream& l);
	void solve() final override;
	double getBestCost() final override;
};

#endif /* ILPALLOCATOR_H */
