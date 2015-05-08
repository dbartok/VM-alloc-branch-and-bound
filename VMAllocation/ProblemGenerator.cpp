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

#include <cmath>
#include <ctime>
#include <fstream>
#include <cstdlib> //MZ

#include "ProblemGenerator.h"
#include "VM.h"
#include "PM.h"

bool ProblemGenerator::randomInitialized = false;

ProblemGenerator::ProblemGenerator(int dim, int nvms, int npms, int minrd, int maxrd, int minrs, int maxrs, int types)
	:dimension(dim), numVMs(nvms), numPMs(npms), minResDemand(minrd), maxResDemand(maxrd), minResSupply(minrs), maxResSupply(maxrs), numPMTypes(types)
{
	if (!randomInitialized) // only initialize random generator once
	{
		randomInitialized = true;
		srand((unsigned int)time(NULL));
	}
}

int ProblemGenerator::randomIntBetween(int min, int max)
{
	int extra = rand() % (max - min + 1); // between 0 and max-min
	return min + extra; // between min and max
}

AllocationProblem ProblemGenerator::generate()
{
	std::vector<VM> VMs;

	// generate VMs
	for (int i = 0; i < numVMs; i++)
	{
		VM vm;
		for (int j = 0; j < dimension; j++)
			vm.demand.push_back(randomIntBetween(minResDemand, maxResDemand));
		vm.initial = randomIntBetween(0, numPMs - 1);
		vm.id = i;

		VMs.push_back(vm);
	}

	std::vector<PM> PMs;
	std::vector<PM> PMTypes;

	// generate PM types
	for (int i = 0; i < numPMTypes; i++)
	{
		PM pm;

		for (int j = 0; j < dimension; j++)
		{
			int cap = randomIntBetween(minResSupply, maxResSupply);
			pm.capacity.push_back(cap);
			pm.resourcesFree.push_back(cap);
		}

		PMTypes.push_back(pm);
	}
	
	// generate PMs
	for (int i = 0; i < numPMs; i++)
	{
		PM pm = PMTypes[randomIntBetween(0, numPMTypes - 1)];
		pm.id = i;

		PMs.push_back(pm);
	}

	AllocationProblem problem;
	problem.PMs = PMs;
	problem.VMs = VMs;
	return problem;
}

AllocationProblem ProblemGenerator::testFromFile(std::string path)
{
	std::ifstream fileIn(path);
	int nVMs;
	int nPMs;
	int buffer;

	// read header
	fileIn >> nVMs;
	fileIn >> nPMs;

	std::vector<VM> VMs;

	//read VMs
	for (int i = 0; i < nVMs; i++)
	{
		VM vm;
		for (int j = 0; j < dimension; j++)
		{
			fileIn >> buffer;
			vm.demand.push_back(buffer);
		}

		fileIn >> buffer;
		vm.initial = buffer;
		vm.id = i;
		VMs.push_back(vm);
	}

	std::vector<PM> PMs;

	// read PMs
	for (int i = 0; i < nPMs; i++)
	{
		PM pm;

		for (int j = 0; j < dimension; j++)
		{
			fileIn >> buffer;
			pm.capacity.push_back(buffer);
			pm.resourcesFree.push_back(buffer);
		}

		pm.id = i;
		PMs.push_back(pm);
	}

	AllocationProblem problem;
	problem.PMs = PMs;
	problem.VMs = VMs;
	return problem;
}
