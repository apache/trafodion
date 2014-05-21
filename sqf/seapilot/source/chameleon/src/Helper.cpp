// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#include "chameleon.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

bool isDirectoryUsable(const string& path)
{
    struct stat status;
    if( stat(path.c_str(), &status) ||	// Get the status of the directory
	    !(status.st_mode & S_IXUSR) ||  // Check whether it can be executed
        !(status.st_mode & S_IWUSR) ||  // Check whether it can be write
        !S_ISDIR(status.st_mode))       // Check whether it is a directory
        return false;

    return true;
}

bool isFileUsable(const string& file)
{
    struct stat status;
    if( stat(file.c_str(), &status) ||  // Get the status of the file
        !(status.st_mode & S_IWUSR) ||  // Check whether it can be write
        S_ISDIR(status.st_mode))        // Check whether it is a file(Not a directory)
        return false;

    return true;
}

string defaultLogPath( )
{
    string path = getenv("MY_SQROOT");
    if(path.empty())
        path = "../../logs/";
    else
        path += "/seapilot/logs/";
    return path;
}
