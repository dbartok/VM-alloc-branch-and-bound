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

#include  <stdlib.h>
#include  <math.h>
#include  <sstream>
#include "IlpAllocator.h"

using std::ofstream;
using std::ifstream;
using std::endl;

IlpAllocator::IlpAllocator(AllocationProblem pr, AllocatorParams pa, std::ofstream& l, solver_type solver)
	:m_problem(pr), m_params(pa), m_log(l), m_solver(solver)
{
	m_numVMs = m_problem.VMs.size();
	m_numPMs = m_problem.PMs.size();
	m_dimension = m_problem.VMs[0].demand.size(); // only works if all VMs have the same number of dimensions
}

void IlpAllocator::create_lp(char *filename)
{
	ofstream ilpfile(filename);

	//Objective function
	if(m_solver==GUROBI)
		ilpfile << "Minimize" << endl;
	if(m_solver==LPSOLVE)
		ilpfile << "min: ";
	for(int i=0;i<m_numPMs;i++)
	{
		if(i>0) ilpfile << " + ";
		ilpfile << COEFF_NR_OF_ACTIVE_HOSTS << " Active_" << i;
	}
	for(int i=0;i<m_numVMs;i++)
	{
		ilpfile << " + " << COEFF_NR_OF_MIGRATIONS << " Migr_" << i;
	}

	if(m_solver==GUROBI)
		ilpfile << endl << endl << "Subject To" << endl;
	if(m_solver==LPSOLVE)
		ilpfile << ";" << endl << endl;

	//Each VM must be allocated to exactly one PM
	for(int j=0;j<m_numVMs;j++)
	{
		for(int i=0;i<m_numPMs;i++)
		{
			if(i>0) ilpfile << " + ";
			ilpfile << "Alloc_" << j << "_" << i;
		}
		if(m_solver==GUROBI)
			ilpfile << " = 1" << endl;
		if(m_solver==LPSOLVE)
			ilpfile << " = 1;" << endl;
	}

	//If a PM hosts at least one VM, then it must be active
	for(int j=0;j<m_numVMs;j++)
	{
		for(int i=0;i<m_numPMs;i++)
		{
			ilpfile << "Alloc_" << j << "_" << i << " - Active_" << i << " <= 0";
			if(m_solver==GUROBI)
				ilpfile << endl;
			if(m_solver==LPSOLVE)
				ilpfile << ";" << endl;
		}
	}

	//Capacity constraints
	for(int d=0;d<m_dimension;d++)
	{
		for(int i=0;i<m_numPMs;i++)
		{
			int pmsize=m_problem.PMs[i].capacity[d];
			ilpfile << "dim_" << d << "_PM_" << i << ": ";
			for(int j=0;j<m_numVMs;j++)
			{
				int vmsize=m_problem.VMs[j].demand[d];
				if(j>0) ilpfile << " + ";
				ilpfile << vmsize << " Alloc_" << j << "_" << i;
			}
			ilpfile << " <= " << pmsize;
			if(m_solver==GUROBI)
				ilpfile << endl;
			if(m_solver==LPSOLVE)
				ilpfile << ";" << endl;
		}
	}

	//Migrations
	for(int j=0;j<m_numVMs;j++)
	{
		int initial=m_problem.VMs[j].initial;
		ilpfile << "Alloc_" << j << "_" << initial << " + Migr_" << j << " = 1";
		if(m_solver==GUROBI)
			ilpfile << endl;
		if(m_solver==LPSOLVE)
			ilpfile << ";" << endl;
	}
	for(int j=0;j<m_numVMs;j++)
	{
		if(j>0) ilpfile << " + ";
		ilpfile << "Migr_" << j;
	}
	ilpfile << " <= " << m_numPMs/m_params.maxMigrationsRatio;
	if(m_solver==GUROBI)
		ilpfile << endl;
	if(m_solver==LPSOLVE)
		ilpfile << ";" << endl;

	//Variables
	if(m_solver==GUROBI)
		ilpfile << endl << "Binary" << endl;
	if(m_solver==LPSOLVE)
		ilpfile << endl << "bin ";
	for(int j=0;j<m_numVMs;j++)
	{
		if(j>0)
		{
			if(m_solver==GUROBI)
				ilpfile << " ";
			if(m_solver==LPSOLVE)
				ilpfile << ", ";
		}
		ilpfile << "Migr_" << j;
	}
	for(int i=0;i<m_numPMs;i++)
	{
		if(m_solver==LPSOLVE)
			ilpfile << ",";
		ilpfile << " Active_" << i;
	}
	for(int j=0;j<m_numVMs;j++)
	{
		for(int i=0;i<m_numPMs;i++)
		{
			if(m_solver==LPSOLVE)
				ilpfile << ",";
			ilpfile << " Alloc_" << j << "_" << i;
		}
	}
	if(m_solver==GUROBI)
		ilpfile << endl << "End" << endl;
	if(m_solver==LPSOLVE)
		ilpfile << ";" << endl;

	ilpfile.close();
}

void IlpAllocator::solveIterative()
{
	std::ostringstream command;
	if(m_solver==GUROBI)
	{
		create_lp("ilp_gurobi.lp");
		command << "gurobi_cl Threads=1 ResultFile=sol_gurobi.sol TimeLimit=" << m_params.timeout << " ilp_gurobi.lp";
	}
	if(m_solver==LPSOLVE)
	{
		create_lp("ilp_lpsolve.lp");
		command << LPSOLVEPATH << " -timeout " << (int)round(m_params.timeout) << " ilp_lpsolve.lp > sol_lpsolve.sol";
	}
	system(command.str().c_str());
}

double IlpAllocator::getOptimum()
{
	double result=-1;
	std::string line;
	if(m_solver==GUROBI)
	{
		ifstream solfile("sol_gurobi.sol");
		std::getline(solfile, line);
		try
		{
			result = atof(line.substr(20).c_str());
		}
		catch (std::out_of_range&)
		{
			// no solution given
		}
//		solfile >> "# Objective value =" >> result;
		solfile.close();
	}
	if(m_solver==LPSOLVE)
	{
		ifstream solfile("sol_lpsolve.sol");
		while(std::getline(solfile, line))
		{
			if(line.find("Value of objective function: ")==0)
			{
				result=atof(line.substr(29).c_str());
				break;
			}
		}
		solfile.close();
	}
	return result;
}

