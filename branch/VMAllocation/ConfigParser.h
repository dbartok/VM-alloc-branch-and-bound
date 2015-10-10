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

#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <vector>
#include <memory>
#include <string>
#include "ProblemGenerator.h"
#include "AllocatorParams.h"
#include "ILPParams.h"
#include "BnBParams.h"

using ParamsPtrVectorType = std::vector<std::shared_ptr<AllocatorParams>>;

class ConfigParser
{
public:
	struct Steps
	{
		int from;
		int to;
		int step;
	};
private:
	std::string m_configFilePath;

	bool showDetailedCost;
	int numTests;

	// generator parameters
	int dimensions;
	Steps VMs;
	Steps PMs;
	int VMmin;
	int VMmax;
	int PMmin;
	int PMmax;
	int numPMtypes;

	// common allocator parameters
	AllocatorType allocatorType;
	std::string name;
	double timeout;

	// ILP only
	SolverType solverType;

	// BnB only
	double boundThreshold;
	int maxMigrationsRatio;
	bool failFirst;
	bool intelligentBound;
	SortType VMSortMethod;
	SortType PMSortMethod;
	bool symmetryBreaking;
	bool initialPMFirst;

	// helpers
	std::unique_ptr<ProblemGenerator> m_generator;
	ParamsPtrVectorType m_paramsList;

	bool getKeyValue(const std::string& line, std::string& key, std::string& value);
	void processAllocator(std::ifstream& configFile);
	void processGeneralParameter(const std::string& key, const std::string& value);
	void processAllocatorParameter(const std::string& key, const std::string& value);
	bool stringToBool(const std::string& toConvert);
public:
	ConfigParser(const std::string& path);
	void parse();
	int getNumTests();
	std::unique_ptr<ProblemGenerator>&& getGenerator();
	ParamsPtrVectorType getParamsList();
	Steps getVMs();
	Steps getPMs();
	bool getShowDetailedCost();
};

#endif