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

#include "BnBAllocator.h"

// preprocess the input problem
void BnBAllocator::preprocess()
{
	if (m_params.intelligentBound)
	{
		// compute number of initial VMs allocated to each PM, and also the number of PMs turned on in the initial assignment (required for bounding)
		for (auto& vm : m_problem.VMs)
		{
			++(vm.initialPM->numAdditionalVMs);
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
bool BnBAllocator::isAllocationValid()
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
void BnBAllocator::allocate(VM* VMHandled, PM* PMCandidate)
{
	assert(m_allocations.find(VMHandled) == m_allocations.end()); // we should only allocate unallocated VMs

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
	m_allocations[VMHandled] = PMCandidate;
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
	if (VMHandled->initialPM != nullptr && PMCandidate != VMHandled->initialPM)
	{
		m_numMigrations++;
	}

	Change change;
	change.VMAllocated = VMHandled;
	change.targetPM = PMCandidate;

	// updating available PMs lists
	for (int vmIndex = 0; vmIndex < m_numVMs; vmIndex++)
	{
		if (m_allocations.find(&m_problem.VMs[vmIndex]) != m_allocations.end()) // VM already allocated, no need to update its available PM list
		{
			continue;
		}

		std::vector<PM*>* availablePMs = &m_problem.VMs[vmIndex].availablePMs;
		std::vector<PM*>::iterator found = std::find(availablePMs->begin(), availablePMs->end(), PMCandidate);

		if (found != availablePMs->end() && !VMFitsInPM(m_problem.VMs[vmIndex], *PMCandidate)) // if the VM fitted onto the PM but doesn't fit anymore
		{
			change.doNotFitAnymore.push_back(&m_problem.VMs[vmIndex]);
			availablePMs->erase(found);
		}
	}

	m_changeStack.push(change);

}

//deallocates a VM
void BnBAllocator::deAllocate(VM* VMHandled)
{
	assert(m_allocations.find(VMHandled) != m_allocations.end()); // we should only deallocate VMs which were allocated

	PM* PMCandidate = m_allocations[VMHandled];

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
	m_allocations.erase(VMHandled);
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
	if (VMHandled->initialPM != nullptr && PMCandidate != VMHandled->initialPM)
	{
		m_numMigrations--;
	}


	Change change = m_changeStack.top();
	m_changeStack.pop();

	assert(*PMCandidate == *(change.targetPM)); // same PM should be saved in the Change as were in the allocation

	for (size_t i = 0; i < change.doNotFitAnymore.size(); i++)
	{
		VM* vmFitsAgain = change.doNotFitAnymore[i];
		std::vector<PM*>::iterator found = std::find(vmFitsAgain->availablePMs.begin(), vmFitsAgain->availablePMs.end(), PMCandidate);
		if (found == vmFitsAgain->availablePMs.end())
		{
			vmFitsAgain->availablePMs.push_back(PMCandidate); // adding the PM to the available PM list
		}
		else
		{
			assert(false); // PM can't already be in the list, because it was removed
		}
	}

}

// returns true if all VMs are allocated
bool BnBAllocator::allVMsAllocated()
{
	return m_VMStack.size() == m_numVMs - 1;
}

// returns true if two PMs should be considered the same in symmetry breaking
bool BnBAllocator::PMsAreTheSame(const PM& pm1, const PM& pm2)
{
	for (int i = 0; i < m_dimension; i++)
	{
		if (!(pm1.capacity[i] == pm2.capacity[i] && pm1.resourcesFree[i] == pm1.capacity[i] && pm2.resourcesFree[i] == pm1.capacity[i]))
			return false;
	}

	return true;
}

// returns true if the VM fits in the PM
bool BnBAllocator::VMFitsInPM(const VM& vm, const PM& pm)
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
VM* BnBAllocator::getNextVM()
{
	// find VM candidate with smallest amount of available values
	if (m_params.failFirst)
	{
		size_t min = INT_MAX;
		VM* minVM = nullptr;
		for (size_t i = 0; i < m_problem.VMs.size(); i++)
		{
			if (m_problem.VMs[i].availablePMs.size() < min && m_allocations.find(&m_problem.VMs[i]) == m_allocations.end()) // return unallocated VM with minimal possible PMs
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
			if (m_allocations.find(&m_problem.VMs[i]) == m_allocations.end()) // return next unallocated VM
			{
				return &m_problem.VMs[i];
			}
		}
	}

	assert(false); // should have returned already
	return nullptr;
}

// initialize PM candidates for every VM
void BnBAllocator::initializePMCandidates()
{
	for (int i = 0; i < m_numVMs; i++)
	{
		m_problem.VMs[i].PMIterator = m_problem.VMs[i].availablePMs.begin();
	}
}

// returns true if current branch is exhausted in the search tree
bool BnBAllocator::currentBranchExhausted(VM* VMHandled)
{
	return (VMHandled->PMIterator == VMHandled->availablePMs.end());
}

// resets PM candidates for a VM
void BnBAllocator::resetCandidates(VM* VMHandled)
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
			if (pm == VMHandled->initialPM)
			{
				pms->erase(pms->begin() + i); // erase
				pms->insert(pms->begin(), pm); // insert to front
				break;
			}
		}
	}

	VMHandled->PMIterator = VMHandled->availablePMs.begin();
}

void BnBAllocator::saveVM(VM* VMHandled)
{
	m_VMStack.push(VMHandled);
}

// backtracks to previous VM and returns it
VM* BnBAllocator::backtrackToPreviousVM()
{
	//stack should never be empty
	assert(!(m_VMStack.empty()));

	VM* top = m_VMStack.top();
	m_VMStack.pop();
	return top;
}

// returns true if all possibilities are exhausted in the search tree
bool BnBAllocator::allPossibilitiesExhausted()
{
	return m_VMStack.empty();
}

// returns next PM candidate for VM
PM* BnBAllocator::getNextPMCandidate(VM* VMHandled)
{
	PM* PMCandidate = *(VMHandled->PMIterator);
	setNextPMCandidate(VMHandled);
	return PMCandidate;
}

// sets next PM candidate for VM (automatically called by getter)
void BnBAllocator::setNextPMCandidate(VM* VMHandled)
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

		} while (PMsAreTheSame(*prevPM, *currPM) && currPM != VMHandled->initialPM); // if this is the initial assignment, don't skip it
	}

	else
	{
		VMHandled->PMIterator++;
	}
}

double BnBAllocator::computeMinimalExtraCost()
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

BnBAllocator::BnBAllocator(AllocationProblem pr, std::shared_ptr<AllocatorParams> pa, std::ofstream& l)
	:m_problem(pr), m_log(l), m_additionalVMCounts(m_problem.VMs.size() + 1, 0)
{
	std::shared_ptr<BnBParams> params = std::dynamic_pointer_cast<BnBParams>(pa);

	if (!params)
		std::cout << "Error: invalid parameters type for BnBAllocator." << std::endl;

	m_params = *params;

	m_numVMs = m_problem.VMs.size();
	m_numPMs = m_problem.PMs.size();
	m_dimension = m_problem.VMs[0].demand.size(); // only works if all VMs have the same number of dimensions

	// computing available migrations
	m_numMaxMigrations = m_numPMs / m_params.maxMigrationsRatio;

	// at the start there are no allocations
	m_numMigrations = 0;
	m_numPMsOn = 0;
	m_bestCostSoFar = INT_MAX;
	m_bestSoFarNumMigrations = INT_MAX;
	m_bestSoFarNumPMsOn = INT_MAX;

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

	// saving initial PM for each VM
	for (auto& vm : m_problem.VMs)
	{
		// newly created VM
		if (vm.initialID == -1)
		{
			vm.initialPM = nullptr;
		}
		// VM has an initial PM
		else
		{
			vm.initialPM = &m_problem.PMs[vm.initialID];
		}
	}

	preprocess();
}

// solves the allocation problem and stores the results in member variables
void BnBAllocator::solve()
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

		if (minimalTotalCost >= m_bestCostSoFar * m_params.boundThreshold) // bound
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
			m_bestCostSoFar = cost;
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

// returns the cost of the best allocation found, or -1 when no allocation was found
double BnBAllocator::getBestCost()
{
	#ifdef VERBOSE_BASIC
		// unfortunately, printing VMs sorted according to ID this way is O(numVMs^2)
		m_log << "alloc:\t";
		for (int i = 0; i < m_numVMs; i++)
		{
			for (const auto& iter : m_bestAllocation)
			{
				if (iter.first->id == i)
				{
					m_log << iter.first->id << "->" << iter.second->id << " ";
					break;
				}
			}
		}
		m_log << std::endl;
	#endif

		return (m_bestAllocation.empty()) ? -1 : m_bestCostSoFar;
}

// computes an initial lower bound for the optimum
// must be called before the start of the algorithm, but after preprocessing
double BnBAllocator::computeInitialLowerBound()
{
	return computeMinimalExtraCost();
}

// get cost components
int BnBAllocator::getActiveHosts()
{
	return m_bestSoFarNumPMsOn;
}

int BnBAllocator::getMigrations()
{
	return m_bestSoFarNumMigrations;
}

const AllocationMapType& BnBAllocator::getBestAllocation()
{
	return m_bestAllocation;
}
