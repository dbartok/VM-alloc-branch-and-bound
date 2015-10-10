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

#ifndef BNBALLOCATOR_H
#define BNBALLOCATOR_H

#include <vector>
#include <memory>
#include <stack>
#include <fstream>

#include "VMAllocator.h"
#include "Change.h"
#include "AllocationProblem.h"
#include "BnBParams.h"
#include "Timer.h"
#include "PM.h"

#define VERBOSE_BASIC // logging configuration, input problem and the solution

// recommended to choose maximum one of the following, else the log files become cramped
//#define VERBOSE_ALG_STEPS // logging steps of the algorithm in a human readable form
//#define VERBOSE_COST_CHANGE // logging how the "best cost so far" changes (with timestamp)

class BnBAllocator : public VMAllocator
{
	AllocationProblem m_problem; // the allocation problem
	BnBParams m_params; // algorithm parameters

	int m_dimension; // dimension of resources
	int m_numVMs; // number of Virtual Machines
	int m_numPMs; // number of Physical Machines

	int m_numAdditionalPMs; // number of additional PMs required if we now leave all VMs on their initial PM
	int m_maxNumVMsOnOnePM; // maximal number of "initial VMs" on one PM (initialized once, but not maintained)
	std::vector<int> m_additionalVMCounts; // maps number of occurences to each "additional VM count"

	std::vector<int> m_allocations; // current allocations
	std::vector<int> m_bestAllocation; // best allocation so far
	int m_numMaxMigrations;
	int m_numMigrations;
	int m_numPMsOn;
	double m_bestSoFar; // best cost so far
	int m_bestSoFarNumMigrations;
	int m_bestSoFarNumPMsOn;

	std::stack<VM*> m_VMStack; // stack of allocated VMs
	std::stack<Change> m_changeStack; // stack of changes during the algorithm

	std::ofstream& m_log; // output log file
	Timer m_timer; // timer for creating timestamps

	void preprocess();
	bool isAllocationValid();
	double computeCost();
	void allocate(VM* VMHandled, PM* PMCandidate);
	void deAllocate(VM* VMHandled);
	bool allVMsAllocated();
	bool PMsAreTheSame(const PM& pm1, const PM& pm2);
	bool VMFitsInPM(const VM& vm, const PM& pm);
	VM* getNextVM();


	void initializePMCandidates();
	bool allPossibilitiesExhausted();
	bool currentBranchExhausted(VM* VMHandled);
	void resetCandidates(VM* VMHandled);
	void saveVM(VM* VMHandled);
	VM* backtrackToPreviousVM();
	PM* getNextPMCandidate(VM* VMHandled);
	void setNextPMCandidate(VM* VMHandled);
	double computeMinimalExtraCost();

public:
	BnBAllocator(AllocationProblem pr, std::shared_ptr<AllocatorParams> pa, std::ofstream& l);
	void solveIterative() final override;
	double computeInitialLowerBound();
	double getOptimum() final override;
	int getActiveHosts();
	int getMigrations();

};

#endif
