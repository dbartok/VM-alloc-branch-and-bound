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

#include "VMAllocator.h"

// preprocess the input problem
void VMAllocator::preprocess()
{
	if (m_params.sortPMs)
	{
		std::sort(m_problem.PMs.begin(), m_problem.PMs.end(), PMComparator);
	}

	if (m_params.sortVMs)
	{
		std::sort(m_problem.VMs.begin(), m_problem.VMs.end(), VMComparator);
	}

#ifdef VERBOSE1 // logging parameter configuration name and problem data
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

// returns the cost of the current allocation
double VMAllocator::computeCost()
{
	//--Number of PMs ON--
	std::vector<bool> PMsOn; // maps to each PM whether it is ON or not

	int numPMsOn = 0; // number of PMs on
	for (int i = 0; i < m_numPMs; i++)
	{
		PMsOn.push_back(false); // every PM is OFF by default
	}

	for (int vm = 0; vm < m_numVMs; vm++)
	{
		int targetPM = m_allocations[vm];
		if (targetPM != -1) // VM is allocated
		{
			if (PMsOn[targetPM] == 0) // this PM has to be turned ON
			{
				numPMsOn++;
			}
			PMsOn[targetPM] = 1;
		}
	}

	//--Number of migrations--
	int numMigrations = 0;
	for (int vm = 0; vm < m_numVMs; vm++)
	{
		int targetPM = m_allocations[vm];
		if (targetPM != -1 && m_problem.PMs[targetPM].id != m_problem.VMs[vm].initial)
		{
			numMigrations++;
		}
	}

	return COEFF_NR_OF_ACTIVE_HOSTS * numPMsOn + COEFF_NR_OF_MIGRATIONS * numMigrations;

}

// allocates a VM to a PM
void VMAllocator::allocate(int VMHandled, int PMCandidate)
{
	m_allocations[VMHandled] = PMCandidate;
	for (int i = 0; i < m_dimension; i++)
		m_problem.PMs[PMCandidate].resourcesFree[i] -= m_problem.VMs[VMHandled].demand[i];

	Change c;
	c.VMAllocated = VMHandled;
	c.targetPM = PMCandidate;

	// updating avaiable PMs lists
	for (int vm = 0; vm < m_numVMs; vm++)
	{
		if (m_allocations[vm] != -1) // VM already allocated, no need to update its available PM list
		{
			continue;
		}

		std::vector<int>& availablePMs = m_problem.VMs[vm].availablePMs;
		std::vector<int>::iterator found = std::find(m_problem.VMs[vm].availablePMs.begin(), m_problem.VMs[vm].availablePMs.end(), PMCandidate);

		if (found != availablePMs.end() && !VMFitsInPM(m_problem.VMs[vm], m_problem.PMs[PMCandidate])) // if the VM fitted onto the PM but doesn't fit anymore
		{
			c.doNotFitAnymore.push_back(vm);
			availablePMs.erase(found);
		}
	}

	m_changeStack.push(c);

}

//deallocates a VM
void VMAllocator::deAllocate(int VMHandled)
{
	int PMCandidate = m_allocations[VMHandled];
	assert(PMCandidate != -1); // we should only deallocate VMs which were allocated

	m_allocations[VMHandled] = -1;
	for (int i = 0; i < m_dimension; i++)
		m_problem.PMs[PMCandidate].resourcesFree[i] += m_problem.VMs[VMHandled].demand[i];
		
	Change c = m_changeStack.top();
	m_changeStack.pop();

	assert(PMCandidate == c.targetPM); // same PM should be saved in the Change as were in the allocation

	for (size_t i = 0; i < c.doNotFitAnymore.size(); i++)
	{
		int vmFitsAgain = c.doNotFitAnymore[i];
		std::vector<int>::iterator found = std::find(m_problem.VMs[vmFitsAgain].availablePMs.begin(), m_problem.VMs[vmFitsAgain].availablePMs.end(), c.targetPM);
		if (found == m_problem.VMs[vmFitsAgain].availablePMs.end())
		{
			m_problem.VMs[vmFitsAgain].availablePMs.push_back(c.targetPM); // adding the PM to the available PM list
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
	return m_VMStack.size() == m_numVMs;
}

// returns true if two PMs should be considered the same in symmetry breaking
bool VMAllocator::PMsAreTheSame(PM pm1, PM pm2)
{
	for (int i = 0; i < m_dimension; i++)
	{
		if (!(pm1.capacity[i] == pm2.capacity[i] && pm1.resourcesFree[i] == pm1.capacity[i] && pm2.resourcesFree[i] == pm1.capacity[i]))
			return false;
	}

	return true;
}

// returns true if the VM fits in the PM
bool VMAllocator::VMFitsInPM(VM vm, PM pm)
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
int VMAllocator::getNextVM(int VMHandled)
{
	m_VMStack.push(VMHandled);

	// find VM candidate with smallest amount of available values
	if (m_params.failFirst)
	{
		size_t min = INT_MAX;
		int minindex = -1;
		for (int i = 0; i < m_numVMs; i++)
		{
			if (m_problem.VMs[i].availablePMs.size() < min && m_allocations[i] == -1) // return unallocated VM with minimal possible PMs
			{
				min = m_problem.VMs[i].availablePMs.size();
				minindex = i;
			}
		}

		return minindex;
	}
	else
	{
		for (int i = 0; i < m_numVMs; i++)
		{
			if (m_allocations[i] == -1) // return next unallocated VM
			{
				return i;
			}
		}
	}

	assert(false); // should have returned already
	return -1;
}

// initialize PM candidates for every VM
void VMAllocator::initializePMCandidates()
{
	for (int i = 0; i < m_numVMs; i++)
	{
		m_problem.VMs[i].PMIterator = m_problem.VMs[i].availablePMs.begin();
	}
}

// returns true if all possibilities are exhausted in the search tree
bool VMAllocator::allPossibilitiesExhausted(int VMHandled)
{
	return (VMHandled < 0);
}

// returns true if current branch is exhausted in the search tree
bool VMAllocator::currentBranchExhausted(int VMHandled)
{
	return (m_problem.VMs[VMHandled].PMIterator == m_problem.VMs[VMHandled].availablePMs.end());
}

// resets PM candidates for a VM
void VMAllocator::resetCandidates(int VMHandled)
{
	std::vector<int>& pms = m_problem.VMs[VMHandled].availablePMs;

	// symmetry breaking -> sorted PMs required
	if (m_params.sortPMsOnTheFly || m_params.symmetryBreaking)
		std::sort(pms.begin(), pms.end());

	// initial PM first -> bringing it to the start of the list
	if (m_params.initialPMFirst)
	{
		for (size_t i = 0; i < pms.size(); i++)
		{
			int PMIterator = pms[i];
			if (m_problem.PMs[PMIterator].id == m_problem.VMs[VMHandled].initial)
			{
				pms.erase(pms.begin() + i); // erase
				pms.insert(pms.begin(), PMIterator); // insert to front
			}
		}
	}

	m_problem.VMs[VMHandled].PMIterator = m_problem.VMs[VMHandled].availablePMs.begin();
}

// backtracks to previous VM and returns it
int VMAllocator::backtrackToPreviousVM()
{
	// -1 is pushed back at first, stack should never be empty
	assert(!(m_VMStack.empty()));

	int top = m_VMStack.top();
	m_VMStack.pop();
	return top;
}

// returns next PM candidate for VM
int VMAllocator::getNextPMCandidate(int VMHandled)
{
	int PMCandidate = *m_problem.VMs[VMHandled].PMIterator;
	setNextPMCandidate(VMHandled);
	return PMCandidate;
}

// sets next PM candidate for VM (automatically called by getter)
void VMAllocator::setNextPMCandidate(int VMHandled)
{
	assert(m_problem.VMs[VMHandled].PMIterator != m_problem.VMs[VMHandled].availablePMs.end()); // there should still be more candidates

	// symmetry breaking, skip same PMs
	if (m_params.symmetryBreaking)
	{
		PM prevPM;
		PM currPM;
		do
		{
			int prevPMIterator = *m_problem.VMs[VMHandled].PMIterator;

			m_problem.VMs[VMHandled].PMIterator++;
			if (m_problem.VMs[VMHandled].PMIterator == m_problem.VMs[VMHandled].availablePMs.end())
			{
				break;
			}

			int currPMIterator = *m_problem.VMs[VMHandled].PMIterator;

			prevPM = m_problem.PMs[prevPMIterator];
			currPM = m_problem.PMs[currPMIterator];
		} while (PMsAreTheSame(prevPM, currPM) && currPM.id != m_problem.VMs[VMHandled].initial); // if this is the initial assignment, don't skip it
	}

	else
	{
		m_problem.VMs[VMHandled].PMIterator++;
	}
}

VMAllocator::VMAllocator(AllocationProblem pr, AllocatorParams pa, std::ofstream& l)
	:m_problem(pr), m_params(pa), m_log(l)
{
	m_numVMs = m_problem.VMs.size();
	m_numPMs = m_problem.PMs.size();
	m_dimension = m_problem.VMs[0].demand.size(); // only works if all VMs have the same number of dimensions

	m_bestSoFar = INT_MAX;

	for (int i = 0; i < m_numVMs; i++)
		m_allocations.push_back(-1); // no allocations at the start

	for (int VM = 0; VM < m_numVMs; VM++)
	{
		for (int PM = 0; PM < m_numPMs; PM++)
		{
			if (VMFitsInPM(m_problem.VMs[VM], m_problem.PMs[PM])) // initialize available PMs list
			{
				m_problem.VMs[VM].availablePMs.push_back(PM);
			}
		}
	}

	preprocess();
}

// solves the allocation problem and stores the results in member variables
void VMAllocator::solveIterative()
{
	int VMHandled = getNextVM(-1); // index of current VM
	initializePMCandidates();
	#ifdef VERBOSE2
		m_log << std::endl << "Starting search..." << std::endl;
	#else
		m_log << std::endl << "Logging disabled." << std::endl;
	#endif
	while (1)
	{
		if (currentBranchExhausted(VMHandled)) // current branch is exhausted
		{
			#ifdef VERBOSE2
				m_log << "Current brach exhausted. ";
			#endif
			VMHandled = backtrackToPreviousVM(); // backtrack to previous VM
			#ifdef VERBOSE2
				m_log << "Backtracked to VM " << VMHandled << ". ";
			#endif
			if (allPossibilitiesExhausted(VMHandled)) // all possibilities exhausted
			{
				#ifdef VERBOSE2
					m_log << "All possibilities exhausted.";
				#endif
				break;
			}
			deAllocate(VMHandled); // undo allocation
			#ifdef VERBOSE2
				m_log << "Deallocated VM " << VMHandled << "."<<std::endl;
			#endif
			continue;
		}

		int PMCandidate = getNextPMCandidate(VMHandled);
		allocate(VMHandled, PMCandidate); // allocate VM
		#ifdef VERBOSE2
			m_log << "Allocated VM " << VMHandled << " to PM " << PMCandidate << ". ";
			m_log << "Current allocation: ";
				for (auto x : m_allocations)
					m_log << x;
		#endif
		assert(isAllocationValid());

		double cost = computeCost();
		#ifdef VERBOSE2
			m_log << "Cost is " << cost << ". "<<std::endl;
		#endif

		if (cost >= m_bestSoFar) // bound
		{
			deAllocate(VMHandled);
			#ifdef VERBOSE2
				m_log << "\tBound. Deallocated VM " << VMHandled << "." << std::endl;
			#endif
			continue;
		}

		if (allVMsAllocated()) // all VMs allocated, updating bestSoFar
		{
			m_bestAllocation = m_allocations;
			m_bestSoFar = cost;
			#ifdef VERBOSE2
				m_log << "\tBest so far updated." << std::endl;
			#endif
			deAllocate(VMHandled);
			#ifdef VERBOSE2
				m_log << "\tAlready at the last VM. Deallocated VM " << VMHandled << "." << std::endl;
			#endif
		}
		else // move down in the tree
		{
			VMHandled = getNextVM(VMHandled);
			resetCandidates(VMHandled);
			#ifdef VERBOSE2
				m_log << "\tMoving down the tree. Next VM is " << VMHandled << "." << std::endl;
			#endif
		}
	}
}

// returns the ost of the optimal allocation
double VMAllocator::getOptimum()
{
#ifdef VERBOSE1
	m_log << std::endl <<"alloc:\t";
	for (auto PM : m_bestAllocation)
	{
		m_log << m_problem.PMs[PM].id << " ";
	}
	m_log << std::endl;
#endif

	return m_bestSoFar;
}