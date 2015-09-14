#include <fstream>
#include <sstream>
#include <iostream>
#include "ConfigParser.h"

ConfigParser::ConfigParser(const std::string& path)
	:m_configFilePath(path)
{

}
int ConfigParser::getNumTests()
{
	return numTests;
}


std::unique_ptr<ProblemGenerator>&& ConfigParser::getGenerator()
{
	return std::move(m_generator);
}

std::vector<AllocatorParams> ConfigParser::getParamsList()
{
	return m_paramsList;
}

void ConfigParser::parse()
{
	std::ifstream configFile(m_configFilePath);

	std::string line;
	while (std::getline(configFile, line))
	{
		if (line == "Allocator{")
		{
			processAllocator(configFile);
		}

		std::string key;
		std::string value;
		if (getKeyValue(line, key, value))
		{
			processGeneralParameter(key, value);
		}		
	}

	m_generator = std::make_unique<ProblemGenerator>
		(	  dimensions
			, VMs
			, PMs
			, VMmin
			, VMmax
			, PMmin
			, PMmax
			, numPMtypes
		);
}

bool ConfigParser::getKeyValue(const std::string& line, std::string& key, std::string& value)
{
	std::istringstream lineStream(line);
	if (!std::getline(lineStream, key, '='))
	{
		return false;
	}
	if (!std::getline(lineStream, value))
	{
		return false;
	}
	return true;
}

void ConfigParser::processGeneralParameter(const std::string& key, const std::string& value)
{
	if (key == "numTests")
	{
		numTests = std::stoi(value);
	}
	else if (key == "dimensions")
	{
		dimensions = std::stoi(value);
	}
	else if (key == "VMs")
	{
		VMs = std::stoi(value);
	}
	else if (key == "PMs")
	{
		PMs = std::stoi(value);
	}
	else if (key == "VMmin")
	{
		VMmin = std::stoi(value);
	}
	else if (key == "VMmax")
	{
		VMmax = std::stoi(value);
	}
	else if (key == "PMmin")
	{
		PMmin = std::stoi(value);
	}
	else if (key == "PMmax")
	{
		PMmax = std::stoi(value);
	}
	else if (key == "numPMtypes")
	{
		numPMtypes = std::stoi(value);
	}
	else
	{
		std::cout << "Invalid key in config file." << std::endl;
		exit(1);
	}
}

void ConfigParser::processAllocator(std::ifstream& configFile)
{
	std::string line;
	while (std::getline(configFile, line))
	{
		if (line == "}")
			break;

		std::string key;
		std::string value;
		if (getKeyValue(line, key, value))
		{
			processAllocatorParameter(key, value);
		}
	}

	AllocatorParams tempParams;
	tempParams.name = name;
	tempParams.timeout = timeout;
	tempParams.boundThreshold = boundThreshold;
	tempParams.maxMigrations = maxMigrations;
	tempParams.failFirst = failFirst;
	tempParams.VMSortMethod = VMSortMethod;
	tempParams.PMSortMethod = PMSortMethod;
	tempParams.symmetryBreaking = symmetryBreaking;
	tempParams.initialPMFirst = initialPMFirst;
	m_paramsList.push_back(tempParams);
}

void ConfigParser::processAllocatorParameter(const std::string& key, const std::string& value)
{
	if (key == "name")
	{
		name = value;
	}
	else if (key == "timeout")
	{
		timeout = std::stoi(value);
	}
	else if (key == "boundThreshold")
	{
		boundThreshold = std::stod(value);
	}
	else if (key == "maxMigrations")
	{
		maxMigrations = std::stoi(value);
	}
	else if (key == "failFirst")
	{
		failFirst = stringToBool(value);
	}
	else if (key == "VMSortMethod")
	{
		VMSortMethod = stringToSortType(value);
	}
	else if (key == "PMSortMethod")
	{
		PMSortMethod = stringToSortType(value);
	}
	else if (key == "symmetryBreaking")
	{
		symmetryBreaking = stringToBool(value);
	}
	else if (key == "initialPMFirst")
	{
		initialPMFirst = stringToBool(value);
	}
}

bool ConfigParser::stringToBool(const std::string& toConvert)
{
	if (toConvert == "true")
		return true;
	return false;
}
