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

/*
The VMAllocation project is currently under development, this version 
is a research prototype intended only for internal usage.
*/

/*
Test configurations can be set up in the this file.
Result files are created in the .\logs folder.
*/

#include <cstdio>
#include <iostream>
#include <vector>
#include <fstream>

#include "VMAllocator.h"
#include "AllocationProblem.h"
#include "ProblemGenerator.h"
#include "Timer.h"
#include "AllocatorParams.h"
#include "Utils.h"

using std::cout;
using std::vector;
using std::ofstream;
using std::endl;

int main()
{
	std::string timeString = currentDateTime();

	ofstream log("logs\\Log_" + timeString + ".txt");
	if (!log.good())
	{
		cout << "Cannot create log files. Maybe the .\\logs folder doesn't exist?" << endl;
		getchar();
		return 1;
	}

	int numTests = 50;
	ProblemGenerator generator
					   (2,		// dimensions
						8,		// VMs
						4,		// PMs
						1,		// VM min
						5,		// VM max
						5,		// PM min
						10,		// PM max
						2);		// num PM types


	Timer t;
	vector <AllocatorParams> paramsList;
	AllocatorParams tempParams;

	// setup parameter configurations
	tempParams.name = "fail first";
	tempParams.failFirst = true;
	tempParams.sortPMs = false;
	tempParams.sortVMs = true;
	tempParams.sortPMsOnTheFly = false;
	tempParams.symmetryBreaking = false;
	tempParams.initialPMFirst = false;
	paramsList.push_back(tempParams);

	tempParams.name = "sortPMsOnTheFly";
	tempParams.sortPMsOnTheFly = true;
	paramsList.push_back(tempParams);

	tempParams.name = "initialPMFirst";
	tempParams.sortPMsOnTheFly = false;
	tempParams.initialPMFirst = true;
	paramsList.push_back(tempParams);

	tempParams.name = "symmetryBreaking";
	tempParams.symmetryBreaking = true;
	tempParams.initialPMFirst = false;
	paramsList.push_back(tempParams);

	tempParams.name = "symmetryBreaking + initialPMFirst";
	tempParams.initialPMFirst = true;
	paramsList.push_back(tempParams);

	// initialize result file
	ofstream output("logs\\Runtimes_" + timeString + ".csv");
	for (unsigned i = 0; i < paramsList.size(); i++) // columns for runtimes
	{
		output << paramsList[i].name;
		output << ", ";
	}
	for (unsigned i = 0; i < paramsList.size(); i++) // columns for costs
	{
		output << paramsList[i].name + ": cost";
		if (i != paramsList.size() - 1)
		{
			output << ", ";
		}
	}
	output << endl;

	// run tests
	cout << "Running " << numTests << " test(s) with " << paramsList.size() << " parameter setups each..."<<endl;

	for (int i = 0; i < numTests; i++) // run for all instances
	{
		cout << "Instance " << i << ":" << endl;
		AllocationProblem problem = generator.generate();
		vector<double> solutions;
		for (unsigned i = 0; i < paramsList.size(); i++) // run current instance for all configurations
		{
			cout << "\t" << paramsList[i].name << "...";
			t.start();
			VMAllocator VMA(problem, paramsList[i], log);
			VMA.solveIterative();
			double elapsed = t.stop();
			cout << " DONE!" << endl;
			output << elapsed;
			output << ", ";
			double opt = VMA.getOptimum();
			solutions.push_back(opt);
#ifdef VERBOSE1			
			log << "Solution = " << opt << endl;
			log << "==============================" << endl;
#endif
		}
		for (unsigned i = 0; i < paramsList.size(); i++)
		{
			output << solutions[i];
			if (i != paramsList.size() - 1)
			{
				output << ", ";
			}
		}
		output << endl;
	}

	cout << "(Finished.)" << endl;
	getchar();
}