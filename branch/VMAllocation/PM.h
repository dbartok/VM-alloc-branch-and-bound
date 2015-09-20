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

#ifndef PM_H
#define PM_H

#include <vector>

struct PM
{
	int id;
	int numAdditionalVMs; // number of additional VMs allocated on this PM, if we now leave all VMs on their initial PM
	std::vector<int> capacity;
	std::vector<int> resourcesFree;

	bool isOn();
	PM();
};

bool operator==(const PM& first, const PM& second);

bool operator==(PM& first, PM& second);

bool LexicographicPMComparator(PM* first, PM* second);

bool MaximumPMComparator(PM* first, PM* second);

bool SumPMComparator(PM* first, PM* second);

#endif