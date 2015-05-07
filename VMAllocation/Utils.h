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

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <ctime>

// returns current date and time in a string format
const std::string currentDateTime() 
{
	time_t timeNow = time(NULL);
	struct tm timeStruct;
	char buf[80];
	localtime_s(&timeStruct, &timeNow);
	strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &timeStruct);

	return buf;
}

#endif