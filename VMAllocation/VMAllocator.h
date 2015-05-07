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

#ifndef VMALLOCATOR_H
#define VMALLOCATOR_H

#include <vector>
#include <stack>
#include <fstream>

#include "Change.h"
#include "AllocationProblem.h"
#include "AllocatorParams.h"

#define COEFF_NR_OF_ACTIVE_HOSTS 2
#define COEFF_NR_OF_MIGRATIONS 1

#define VERBOSE1
//#define VERBOSE2

class VMAllocator
{
	AllocationProblem m_problem; // the allocation problem
	AllocatorParams m_params; // algorithm parameters

	int m_dimension; // dimension of resources
	int m_numVMs; // number of Virtual Machines
	int m_numPMs; // number of Physical Machines

	std::vector<int> m_allocations; // current allocations
	std::vector<int> m_bestAllocation; // best allocation so far
	double m_bestSoFar; // best cost so far

	std::vector<int> m_currentPMCandidateAtVM; // maps next candidate PM to every VM
	std::stack<int> m_VMStack; // stack of allocated VMs
	std::stack<Change> m_changeStack; // stack of changes during the algorithm

	std::ofstream& m_log; // output log file

	void preprocess();
	bool isAllocationValid();
	double computeCost();
	void allocate(int VMHandled, int PMCandidate);
	void deAllocate(int VMHandled);
	bool allVMsAllocated();
	bool PMsAreTheSame(PM pm1, PM pm2);
	bool VMFitsInPM(VM vm, PM pm);
	int getNextVM(int VMHandled);


	void initializePMCandidates();
	bool allPossibilitiesExhausted(int VMHandled);
	bool currentBranchExhausted(int VMHandled);
	void resetCandidates(int VMHandled);
	int backtrackToPreviousVM();
	int getNextPMCandidate(int VMHandled);
	void setNextPMCandidate(int VMHandled);


public:
	VMAllocator(AllocationProblem pr, AllocatorParams pa, std::ofstream& l);
	void solveIterative();
	double getOptimum();

};

#endif