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
#include <climits>

#include "VMAllocator.h"
#include "IlpAllocator.h"
#include "AllocationProblem.h"
#include "ProblemGenerator.h"
#include "Timer.h"
#include "AllocatorParams.h"
#include "Utils.h"
#include "ConfigParser.h"

using std::cout;
using std::vector;
using std::ofstream;
using std::endl;

int main()
{
	std::string timeString = currentDateTime();
	#ifdef WIN32
		ofstream log("logs\\Log_" + timeString + ".txt");
	#else
		ofstream log("logs/Log_" + timeString + ".txt");
	#endif
	if (!log.good())
	{
		cout << "Cannot create log files. Maybe the .\\logs folder doesn't exist?" << endl;
		getchar();
		return 1;
	}

	Timer t;

	ConfigParser parser("config.txt");
	parser.parse();
	std::unique_ptr<ProblemGenerator> generator = parser.getGenerator();
	vector <AllocatorParams> paramsList = parser.getParamsList();

	// initialize result file
	#ifdef WIN32
		ofstream output("logs\\Runtimes_" + timeString + ".csv");
	#else
		ofstream output("logs/Runtimes_" + timeString + ".csv");
	#endif
	for (unsigned i = 0; i < paramsList.size(); i++) // columns for runtimes
	{
		output << paramsList[i].name;
		output << "; ";
	}
	for (unsigned i = 0; i < paramsList.size(); i++) // columns for costs
	{
		output << paramsList[i].name + ": cost";
//		if (i != paramsList.size() - 1)
//		{
			output << "; ";
//		}
	}

#ifndef WIN32
	output << "Gurobi cost; Lpsolve cost";
#endif
	output << endl;

	// run tests
	cout << "Running " << parser.getNumTests() << " test(s) with " << paramsList.size() << " parameter setups each..." << endl;

	for (int i = 0; i < parser.getNumTests(); i++) // run for all instances
	{
		cout << "Instance " << i << ":" << endl;
		log << "Instance " << i << ":" << endl;
		AllocationProblem problem = generator->generate_ff();
		vector<double> solutions;
		for (unsigned i = 0; i < paramsList.size(); i++) // run current instance for all configurations
		{
			cout << "\t" << paramsList[i].name << "...";
			t.start();
			VMAllocator VMA(problem, paramsList[i], log);
			VMA.solveIterative();
			double elapsed = t.getElapsedTime();
			cout << " DONE!" << endl;
			output << elapsed;
			output << "; ";
			double opt = VMA.getOptimum();
			solutions.push_back(opt);
#ifdef VERBOSE_BASIC			
			log << "Solution = " << opt << endl;
			log << "==============================" << endl;
#endif
		}

		for (unsigned i = 0; i < paramsList.size(); i++)
		{
			output << solutions[i];
//			if (i != paramsList.size() - 1)
//			{
				output << "; ";
//			}
		}

#ifndef WIN32
		IlpAllocator IA1(problem, paramsList[0], log, GUROBI);
		IA1.solveIterative();
		double opt = IA1.getOptimum();
		output << opt << "; ";
		IlpAllocator IA2(problem, paramsList[0], log, LPSOLVE);
		IA2.solveIterative();
		opt = IA2.getOptimum();
		output << opt;
#endif
		output << endl;
	}

	output.close();
	log.close();
	cout << "(Finished.)" << endl;
	//getchar();
}
