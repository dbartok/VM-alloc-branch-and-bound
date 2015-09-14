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

class ConfigParser
{
private:
	std::string m_configFilePath;

	int numTests;

	int dimensions;
	int VMs;
	int PMs;
	int VMmin;
	int VMmax;
	int PMmin;
	int PMmax;
	int numPMtypes;

	std::string name;
	double timeout;
	double boundThreshold;
	int maxMigrations;
	bool failFirst;
	SortType VMSortMethod;
	SortType PMSortMethod;
	bool symmetryBreaking;
	bool initialPMFirst;

	std::unique_ptr<ProblemGenerator> m_generator;
	std::vector<AllocatorParams> m_paramsList;

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
	std::vector<AllocatorParams> getParamsList();
};

#endif