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

#include <vector>
#include <algorithm>
#include <functional>
#include <stack>
#include <cassert>
#include <iostream>
#include <climits>

#include "VMAllocator.h"

// preprocess the input problem
void VMAllocator::preprocess()
{
#ifdef VERBOSE_BASIC // logging parameter configuration name and problem data
	m_log << "Parameter configuration: " << m_params.name << std::endl;
	m_log << std::endl << "PMs:\t";
	for (auto pm : m_problem.PMs)
	{
		m_log << "[";
		for (int i = 0; i < m_dimension; i++)
		{
			m_log << pm.capacity[i];
			if (i != m_dimension - 1)
				m_log << " ";
		}

		m_log << "] ";
	}
	m_log << std::endl << "VMs:\t";
	for (auto vm : m_problem.VMs)
	{
		m_log << "[";
		for (int i = 0; i < m_dimension; i++)
		{
			m_log << vm.demand[i];
			if (i != m_dimension - 1)
				m_log << " ";
		}

		m_log << "] ";
	}
	m_log << std::endl << "init:\t";
	for (auto vm : m_problem.VMs)
	{
		m_log << vm.initial << " ";
	}
	m_log << std::endl;
#endif

	if (m_params.intelligentBound)
	{
		// compute number of initial VMs allocated to each PM, and also the number of PMs turned on in the initial assignment (required for bounding)
		for (auto& vm : m_problem.VMs)
		{
			assert(vm.initial == m_problem.PMs[vm.initial].id); // PMs must be unsorted
			vm.initialPM = &m_problem.PMs[vm.initial]; // saving initial PM

			++(m_problem.PMs[vm.initial].numAdditionalVMs);
		}
		m_numAdditionalPMs = std::count_if(m_problem.PMs.cbegin(), m_problem.PMs.cend(), [](const PM& pm) {return pm.numAdditionalVMs > 0; });
		m_maxNumVMsOnOnePM = std::max_element(m_problem.PMs.cbegin(), m_problem.PMs.cend(), 
		[](const PM& pm1, const PM& pm2)
		{
			return pm1.numAdditionalVMs < pm2.numAdditionalVMs;
		})->numAdditionalVMs;

		// building map of additional VM counts
		for (const auto& pm : m_problem.PMs)
		{
			++(m_additionalVMCounts[pm.numAdditionalVMs]);
		}

		#ifdef VERBOSE_ALG_STEPS
			m_log << "\tMaximal number of VMs on one PM: " << m_maxNumVMsOnOnePM << std::endl;
			m_log << "\tInitial additional PMs: " << m_numAdditionalPMs << std::endl;
			m_log << "\tInitial additional Vms on each PM: ";
			for (auto x : m_problem.PMs)
				m_log << x.id << ":" << x.numAdditionalVMs << " ";
			m_log << std::endl;
			m_log << "\tInitial additional VM counts: ";
			for (auto x : m_additionalVMCounts)
				m_log << x << " ";
			m_log << std::endl;
		#endif
	}
	// sorting VMs
	switch (m_params.VMSortMethod)
	{
	case NONE:
		break;
	case LEXICOGRAPHIC:
		std::sort(m_problem.VMs.begin(), m_problem.VMs.end(), LexicographicVMComparator);
		break;
	case MAXIMUM:
		std::sort(m_problem.VMs.begin(), m_problem.VMs.end(), MaximumVMComparator);
		break;
	case SUM:
		std::sort(m_problem.VMs.begin(), m_problem.VMs.end(), SumVMComparator);
		break;
	default:
		assert(false); // the enum has to take some value
		break;
	}
}


// returns true if the current allocation is valid
bool VMAllocator::isAllocationValid()
{
	for (int pm = 0; pm < m_numPMs; pm++)
	{
		for (int i = 0; i < m_dimension; i++)
			if (m_problem.PMs[pm].resourcesFree[i] < 0) // constraint violated
			{
				return false;
			}
	}
	return true;
}

// allocates a VM to a PM
void VMAllocator::allocate(VM* VMHandled, PM* PMCandidate)
{
	assert(m_allocations[VMHandled->id] == -1); // we should only allocate unallocated VMs

	//--Turning on a PM--
	if (!(PMCandidate->isOn()))
	{
		m_numPMsOn++;

		if (m_params.intelligentBound)
		{
			// this PM is turned on, cannot be emptied, remove it from the map
			--(m_additionalVMCounts[PMCandidate->numAdditionalVMs]);

			// decrease global counter if this PM was previously emptiable
			if (PMCandidate->numAdditionalVMs > 0)
			{
				--m_numAdditionalPMs;
			}
		}
	}

	// reserve resources
	m_allocations[VMHandled->id] = PMCandidate->id;
	for (int i = 0; i < m_dimension; i++)
		PMCandidate->resourcesFree[i] -= VMHandled->demand[i];

	if (m_params.intelligentBound)
	{
		// handling initial PM of the allocated VM
		// first we check if it is emptiable
		if (!VMHandled->initialPM->isOn())
		{
			int& numVMs = VMHandled->initialPM->numAdditionalVMs; // saving number of initial VMs remaining on this PM

			// modifying the map: there is now one less initial VM on this PM
			--(m_additionalVMCounts[numVMs]);
			++(m_additionalVMCounts[numVMs - 1]);

			if (numVMs == 1) // this is the last additional VM on the PM, the PM becomes non-emptiable (because it is going to be empty)
			{
				--m_numAdditionalPMs;
			}

			// we also have to decrease the counter for the PM
			--(numVMs);
		}

		#ifdef VERBOSE_ALG_STEPS
			m_log << "\tCurrent additional PMs: " << m_numAdditionalPMs << std::endl;
			m_log << "\tCurrent additional VMs on each PM: ";
			for (auto x : m_problem.PMs)
				m_log << x.id << ":" << x.numAdditionalVMs << " ";
			m_log << std::endl;
			m_log << "\tCurrent additional VM counts: ";
			for (auto x : m_additionalVMCounts)
				m_log << x << " ";
			m_log << std::endl;
		#endif
	}

	//--Migrating a VM--
	if (VMHandled->initial != -1 && PMCandidate->id != VMHandled->initial)
	{
		m_numMigrations++;
	}

	Change change;
	change.VMAllocated = VMHandled;
	change.targetPM = PMCandidate;

	// updating available PMs lists
	for (int vm = 0; vm < m_numVMs; vm++)
	{
		if (m_allocations[m_problem.VMs[vm].id] != -1) // VM already allocated, no need to update its available PM list
		{
			continue;
		}

		std::vector<PM*>* availablePMs = &m_problem.VMs[vm].availablePMs;
		std::vector<PM*>::iterator found = std::find(availablePMs->begin(), availablePMs->end(), PMCandidate);

		if (found != availablePMs->end() && !VMFitsInPM(m_problem.VMs[vm], *PMCandidate)) // if the VM fitted onto the PM but doesn't fit anymore
		{
			change.doNotFitAnymore.push_back(vm);
			availablePMs->erase(found);
		}
	}

	m_changeStack.push(change);

}

//deallocates a VM
void VMAllocator::deAllocate(VM* VMHandled)
{
	int PMCandidateIndex = m_allocations[VMHandled->id];
	assert(PMCandidateIndex != -1); // we should only deallocate VMs which were allocated

	PM* PMCandidate = &m_problem.PMs[PMCandidateIndex];

	if (m_params.intelligentBound)
	{
		// handling initial PM of the allocated VM
		// first we check if it is emptiable
		if (!VMHandled->initialPM->isOn())
		{
			int& numVMs = VMHandled->initialPM->numAdditionalVMs;

			// modifying the map: there is now one more initial VM on this PM
			--(m_additionalVMCounts[numVMs]);
			++(m_additionalVMCounts[numVMs + 1]);

			if (numVMs == 0) // this is going to be the first additional VM on the PM, the PM becomes emptiable (previously it was be empty)
			{
				++m_numAdditionalPMs;
			}

			// we also have to increase the counter for the PM
			++(numVMs);
		}
	}

	// free resources
	m_allocations[VMHandled->id] = -1;
	for (int i = 0; i < m_dimension; i++)
		PMCandidate->resourcesFree[i] += VMHandled->demand[i];

	//--Turning on a PM--
	if (!(PMCandidate->isOn()))
	{
		m_numPMsOn--;

		if (m_params.intelligentBound)
		{
			// this PM is turned off, it can be emptied, add it to the map
			++(m_additionalVMCounts[PMCandidate->numAdditionalVMs]);

			// increase global counter if this PM is now emptiable (it has initial VMs on it)
			if (PMCandidate->numAdditionalVMs > 0)
			{
				++m_numAdditionalPMs;
			}
		}
	}
	if (m_params.intelligentBound)
	{
		#ifdef VERBOSE_ALG_STEPS
			m_log << "\tCurrent additional PMs: " << m_numAdditionalPMs << std::endl;
			m_log << "\tCurrent additional VMs on each PM: ";
			for (auto x : m_problem.PMs)
				m_log << x.id << ":" << x.numAdditionalVMs << " ";
			m_log << std::endl;
			m_log << "\tCurrent additional VM counts: ";
			for (auto x : m_additionalVMCounts)
				m_log << x << " ";
			m_log << std::endl;
		#endif
	}

	//--Migrating a VM--
	if (VMHandled->initial != -1 && PMCandidate->id != VMHandled->initial)
	{
		m_numMigrations--;
	}
	
		
	Change change = m_changeStack.top();
	m_changeStack.pop();

	assert(*PMCandidate == *(change.targetPM)); // same PM should be saved in the Change as were in the allocation

	for (size_t i = 0; i < change.doNotFitAnymore.size(); i++)
	{
		int vmFitsAgain = change.doNotFitAnymore[i];
		std::vector<PM*>::iterator found = std::find(m_problem.VMs[vmFitsAgain].availablePMs.begin(), m_problem.VMs[vmFitsAgain].availablePMs.end(), PMCandidate);
		if (found == m_problem.VMs[vmFitsAgain].availablePMs.end())
		{
			m_problem.VMs[vmFitsAgain].availablePMs.push_back(PMCandidate); // adding the PM to the available PM list
		}
		else
		{
			assert(false); // PM can't already be in the list, because it was removed
		}
	}
	
}

// returns true if all VMs are allocated
bool VMAllocator::allVMsAllocated()
{
	return m_VMStack.size() == m_numVMs - 1;
}

// returns true if two PMs should be considered the same in symmetry breaking
bool VMAllocator::PMsAreTheSame(const PM& pm1, const PM& pm2)
{
	for (int i = 0; i < m_dimension; i++)
	{
		if (!(pm1.capacity[i] == pm2.capacity[i] && pm1.resourcesFree[i] == pm1.capacity[i] && pm2.resourcesFree[i] == pm1.capacity[i]))
			return false;
	}

	return true;
}

// returns true if the VM fits in the PM
bool VMAllocator::VMFitsInPM(const VM& vm, const PM& pm)
{
	for (int i = 0; i < m_dimension; i++)
	{
		if (pm.resourcesFree[i] < vm.demand[i])
		{
			return false;
		}
	}

	return true;
}

// returns the next VM
VM* VMAllocator::getNextVM()
{
	// find VM candidate with smallest amount of available values
	if (m_params.failFirst)
	{
		size_t min = INT_MAX;
		VM* minVM = nullptr;
		for (size_t i = 0; i < m_problem.VMs.size(); i++)
		{
			if (m_problem.VMs[i].availablePMs.size() < min && m_allocations[m_problem.VMs[i].id] == -1) // return unallocated VM with minimal possible PMs
			{
				min = m_problem.VMs[i].availablePMs.size();
				minVM = &m_problem.VMs[i];
			}
		}

		return minVM;
	}
	else
	{
		for (size_t i = 0; i < m_problem.VMs.size(); i++)
		{
			if (m_allocations[m_problem.VMs[i].id] == -1) // return next unallocated VM
			{
				return &m_problem.VMs[i];
			}
		}
	}

	assert(false); // should have returned already
	return nullptr;
}

// initialize PM candidates for every VM
void VMAllocator::initializePMCandidates()
{
	for (int i = 0; i < m_numVMs; i++)
	{
		m_problem.VMs[i].PMIterator = m_problem.VMs[i].availablePMs.begin();
	}
}

// returns true if current branch is exhausted in the search tree
bool VMAllocator::currentBranchExhausted(VM* VMHandled)
{
	return (VMHandled->PMIterator == VMHandled->availablePMs.end());
}

// resets PM candidates for a VM
void VMAllocator::resetCandidates(VM* VMHandled)
{
	std::vector<PM*>* pms = &(VMHandled->availablePMs);

	switch (m_params.PMSortMethod)
	{
	case NONE:
		if (m_params.symmetryBreaking) 	// symmetry breaking -> sorted PMs required anyway
			std::sort(pms->begin(), pms->end(), LexicographicPMComparator);
		break;
	case LEXICOGRAPHIC:
		std::sort(pms->begin(), pms->end(), LexicographicPMComparator);
		break;
	case MAXIMUM:
		std::sort(pms->begin(), pms->end(), MaximumPMComparator);
		break;
	case SUM:
		std::sort(pms->begin(), pms->end(), SumPMComparator);
		break;
	default:
		assert(false); // the enum has to take some value
		break;
	}

	// initial PM first -> bringing it to the start of the list
	if (m_params.initialPMFirst)
	{
		for (size_t i = 0; i < pms->size(); i++)
		{
			PM* pm = (*pms)[i];
			if (pm->id == VMHandled->initial)
			{
				pms->erase(pms->begin() + i); // erase
				pms->insert(pms->begin(), pm); // insert to front
			}
		}
	}

	VMHandled->PMIterator = VMHandled->availablePMs.begin();
}

void VMAllocator::saveVM(VM* VMHandled)
{
	m_VMStack.push(VMHandled);
}

// backtracks to previous VM and returns it
VM* VMAllocator::backtrackToPreviousVM()
{
	//stack should never be empty
	assert(!(m_VMStack.empty()));

	VM* top = m_VMStack.top();
	m_VMStack.pop();
	return top;
}

// returns true if all possibilities are exhausted in the search tree
bool VMAllocator::allPossibilitiesExhausted()
{
	return m_VMStack.empty();
}

// returns next PM candidate for VM
PM* VMAllocator::getNextPMCandidate(VM* VMHandled)
{
	PM* PMCandidate = *(VMHandled->PMIterator);
	setNextPMCandidate(VMHandled);
	return PMCandidate;
}

// sets next PM candidate for VM (automatically called by getter)
void VMAllocator::setNextPMCandidate(VM* VMHandled)
{
	assert(VMHandled->PMIterator != VMHandled->availablePMs.end()); // there should still be more candidates

	// symmetry breaking, skip same PMs
	if (m_params.symmetryBreaking)
	{
		PM* prevPM;
		PM* currPM;
		do
		{
			prevPM = *(VMHandled->PMIterator);

			VMHandled->PMIterator++;
			if (VMHandled->PMIterator == VMHandled->availablePMs.end())
			{
				break;
			}

			currPM = *(VMHandled->PMIterator);

		} while (PMsAreTheSame(*prevPM, *currPM) && currPM->id != VMHandled->initial); // if this is the initial assignment, don't skip it
	}

	else
	{
		VMHandled->PMIterator++;
	}
}

double VMAllocator::computeMinimalExtraCost()
{
	int remainingMigrations = m_numMaxMigrations - m_numMigrations;

	int minimalExtraCost = m_numAdditionalPMs * COEFF_NR_OF_ACTIVE_HOSTS;
	int migrationsDone = 0;

	for (int numVMs = 1; numVMs <= m_maxNumVMsOnOnePM; ++numVMs)
	{
		if (numVMs >= COEFF_NR_OF_ACTIVE_HOSTS / COEFF_NR_OF_MIGRATIONS)
			break;
		int numPMsEmptied = std::min(m_additionalVMCounts[numVMs], (remainingMigrations - migrationsDone) / numVMs);
		migrationsDone += numPMsEmptied * numVMs;
		minimalExtraCost -= (numPMsEmptied * COEFF_NR_OF_ACTIVE_HOSTS - numPMsEmptied * numVMs * COEFF_NR_OF_MIGRATIONS);
	}

	return minimalExtraCost;
}

VMAllocator::VMAllocator(AllocationProblem pr, AllocatorParams pa, std::ofstream& l)
	:m_problem(pr), m_params(pa), m_log(l), m_additionalVMCounts(m_problem.VMs.size()+1, 0)
{
	m_numVMs = m_problem.VMs.size();
	m_numPMs = m_problem.PMs.size();
	m_dimension = m_problem.VMs[0].demand.size(); // only works if all VMs have the same number of dimensions

	// computing available migrations
	m_numMaxMigrations = m_numPMs / m_params.maxMigrationsRatio;

	// at the start there are no allocations
	m_numMigrations = 0; 
	m_numPMsOn = 0;
	m_bestSoFar = INT_MAX;
	m_bestSoFarNumMigrations = INT_MAX;
	m_bestSoFarNumPMsOn = INT_MAX;

	for (int i = 0; i < m_numVMs; i++)
		m_allocations.push_back(-1); // no allocations at the start

	for (int vm = 0; vm < m_numVMs; vm++)
	{
		for (int pm = 0; pm < m_numPMs; pm++)
		{
			if (VMFitsInPM(m_problem.VMs[vm], m_problem.PMs[pm])) // initialize available PMs list
			{
				m_problem.VMs[vm].availablePMs.push_back(&m_problem.PMs[pm]);
			}
		}
	}

	preprocess();
}

// solves the allocation problem and stores the results in member variables
void VMAllocator::solveIterative()
{
	m_timer.start();

	VM* VMHandled = getNextVM(); // index of current VM
	initializePMCandidates();

	#ifdef VERBOSE_ALG_STEPS
		m_log << std::endl << "Starting search..." << std::endl;
	#endif

	while (1)
	{
		if (m_timer.getElapsedTime() > m_params.timeout) // check for timeout
		{
			#ifdef VERBOSE_BASIC
				m_log << "TIMED OUT." << std::endl;
			#endif
				break;
		}

		if (currentBranchExhausted(VMHandled)) // current branch is exhausted
		{
			#ifdef VERBOSE_ALG_STEPS
				m_log << "Current brach exhausted. ";
			#endif
			if (allPossibilitiesExhausted()) // all possibilities exhausted
			{
			#ifdef VERBOSE_ALG_STEPS
				m_log << "All possibilities exhausted.";
			#endif
				break;
			}
			VMHandled = backtrackToPreviousVM(); // backtrack to previous VM
			#ifdef VERBOSE_ALG_STEPS
				m_log << "Backtracked to VM " << VMHandled->id << ". ";
			#endif
			deAllocate(VMHandled); // undo allocation
			#ifdef VERBOSE_ALG_STEPS
				m_log << "Deallocated VM " << VMHandled->id << ". ";
				m_log << "Current allocation: ";
				for (auto x : m_allocations)
					m_log << x << " ";
				m_log << std::endl;
			#endif
			continue;
		}

		PM* PMCandidate = getNextPMCandidate(VMHandled);
		allocate(VMHandled, PMCandidate); // allocate VM
		#ifdef VERBOSE_ALG_STEPS
			m_log << "Allocated VM " << VMHandled->id << " to PM " << PMCandidate->id << ". ";
			m_log << "Current allocation: ";
				for (auto x : m_allocations)
					m_log << x << " ";
			m_log << " -> ";
		#endif
		assert(isAllocationValid());

		if (m_numMigrations > m_numMaxMigrations) // ran out of migrations
		{
			deAllocate(VMHandled);
			#ifdef VERBOSE_ALG_STEPS
				m_log << "\tToo many migrations. Deallocated VM " << VMHandled->id << "." << std::endl;
				m_log << "Current allocation: ";
				for (auto x : m_allocations)
					m_log << x << " ";
				m_log << std::endl;
			#endif
			continue;
		}

		double cost = COEFF_NR_OF_ACTIVE_HOSTS * m_numPMsOn + COEFF_NR_OF_MIGRATIONS * m_numMigrations;
		#ifdef VERBOSE_ALG_STEPS
		m_log << "numPMsOn = " << m_numPMsOn << ", numMigrations = " << m_numMigrations <<", cost is: "<< cost << ". " << std::endl;
		#endif

		double minimalTotalCost = cost;

		if (m_params.intelligentBound)
		{
			double extraCost = computeMinimalExtraCost();
			minimalTotalCost += extraCost;
			#ifdef VERBOSE_ALG_STEPS
			m_log << "Computed minimal extra cost = " << extraCost << ", minimal total cost = " << minimalTotalCost << std::endl;
			#endif
		}

		if (minimalTotalCost >= m_bestSoFar * m_params.boundThreshold) // bound
		{
			deAllocate(VMHandled);
			#ifdef VERBOSE_ALG_STEPS
				m_log << "\tBound. Deallocated VM " << VMHandled->id << "." << std::endl;
				m_log << "Current allocation: ";
				for (auto x : m_allocations)
					m_log << x << " ";
				m_log << std::endl;
			#endif
			continue;
		}

		if (allVMsAllocated()) // all VMs allocated, updating bestSoFar
		{
			m_bestAllocation = m_allocations;
			m_bestSoFar = cost;
			m_bestSoFarNumPMsOn = m_numPMsOn;
			m_bestSoFarNumMigrations = m_numMigrations;
			#ifdef VERBOSE_COST_CHANGE
				m_log << m_timer.getElapsedTime() << ", " << cost << std::endl;
			#endif
			#ifdef VERBOSE_ALG_STEPS
				m_log << "\tBest so far updated." << std::endl;
			#endif
			deAllocate(VMHandled);
			#ifdef VERBOSE_ALG_STEPS
				m_log << "\tAlready at the last VM. Deallocated VM " << VMHandled->id << "." << std::endl;
				m_log << "Current allocation: ";
				for (auto x : m_allocations)
					m_log << x << " ";
				m_log << std::endl;
			#endif
		}
		else // move down in the tree
		{
			saveVM(VMHandled);
			VMHandled = getNextVM();
			resetCandidates(VMHandled);
			#ifdef VERBOSE_ALG_STEPS
				m_log << "\tMoving down the tree. Next VM is " << VMHandled->id << "." << std::endl;
			#endif
		}
	}
}

// returns the cost of the optimal allocation
double VMAllocator::getOptimum()
{
	#ifdef VERBOSE_BASIC
		m_log << std::endl <<"alloc:\t";
		for (auto PM : m_bestAllocation)
		{
			m_log << m_problem.PMs[PM].id << " ";
		}
		m_log << std::endl;
	#endif

	return m_bestSoFar;
}

// computes an initial lower bound for the optimum
// must be called before the start of the algorithm, but after preprocessing
double VMAllocator::computeInitialLowerBound()
{
	return computeMinimalExtraCost();
}

// get cost components
int VMAllocator::getActiveHosts()
{
	return m_bestSoFarNumPMsOn;
}

int VMAllocator::getMigrations()
{
	return m_bestSoFarNumMigrations;
}
