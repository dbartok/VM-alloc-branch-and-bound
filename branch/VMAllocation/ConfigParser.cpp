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

ParamsPtrVectorType ConfigParser::getParamsList()
{
	return m_paramsList;
}

ConfigParser::Steps ConfigParser::getVMs()
{
	return VMs;
}

ConfigParser::Steps ConfigParser::getPMs()
{
	return PMs;
}

bool ConfigParser::getShowDetailedCost()
{
	return showDetailedCost;
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
	if (key == "showDetailedCost")
	{
		showDetailedCost = stringToBool(value);
	}
	else if (key == "numTests")
	{
		numTests = std::stoi(value);
	}
	else if (key == "dimensions")
	{
		dimensions = std::stoi(value);
	}
	else if (key == "VMsFrom")
	{
		VMs.from = std::stoi(value);
	}
	else if (key == "VMsTo")
	{
		VMs.to = std::stoi(value);
	}
	else if (key == "VMsStep")
	{
		VMs.step = std::stoi(value);
	}
	else if (key == "PMsFrom")
	{
		PMs.from = std::stoi(value);
	}
	else if (key == "PMsTo")
	{
		PMs.to = std::stoi(value);
	}
	else if (key == "PMsStep")
	{
		PMs.step = std::stoi(value);
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
		std::cout << "Invalid key in config file: "<< key << std::endl;
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

	std::shared_ptr<AllocatorParams> tempParams;

	if (allocatorType == BnB)
	{
		tempParams = std::make_shared<BnBParams>();
	}
	else if (allocatorType == ILP)
	{
		tempParams = std::make_shared<ILPParams>();
	}
	
	tempParams->allocatorType = allocatorType;
	tempParams->name = name;
	tempParams->timeout = timeout;
	tempParams->maxMigrationsRatio = maxMigrationsRatio;


	std::shared_ptr<BnBParams> bnbParams = std::dynamic_pointer_cast<BnBParams>(tempParams);

	if (bnbParams)
	{
		bnbParams->boundThreshold = boundThreshold;
		bnbParams->failFirst = failFirst;
		bnbParams->intelligentBound = intelligentBound;
		bnbParams->VMSortMethod = VMSortMethod;
		bnbParams->PMSortMethod = PMSortMethod;
		bnbParams->symmetryBreaking = symmetryBreaking;
		bnbParams->initialPMFirst = initialPMFirst;
	}

	std::shared_ptr<ILPParams> ilpParams = std::dynamic_pointer_cast<ILPParams>(tempParams);

	if (ilpParams)
	{
		ilpParams->solverType = solverType;
	}

	m_paramsList.push_back(tempParams);
}

void ConfigParser::processAllocatorParameter(const std::string& key, const std::string& value)
{
	if (key == "name")
	{
		name = value;
	}
	else if (key == "allocatorType")
	{
		allocatorType = stringToAllocatorType(value);
	}
	else if (key == "timeout")
	{
		timeout = std::stoi(value);
	}
	else if (key == "solverType")
	{
		solverType = stringToSolverType(value);
	}
	else if (key == "boundThreshold")
	{
		boundThreshold = std::stod(value);
	}
	else if (key == "maxMigrationsRatio")
	{
		maxMigrationsRatio = std::stoi(value);
	}
	else if (key == "failFirst")
	{
		failFirst = stringToBool(value);
	}
	else if (key == "intelligentBound")
	{
		intelligentBound = stringToBool(value);
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
