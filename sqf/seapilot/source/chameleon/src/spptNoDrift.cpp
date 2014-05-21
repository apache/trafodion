// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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

#include <sys/types.h>
#include <algorithm>  
#include "spptNoDrift.h"
#include <iostream>

using namespace std;
const time_t USECS_IN_ONE_SEC=1000000;

noDrift::noDrift()
{
    started_ = false;
}

noDrift::~noDrift()
{

}

void noDrift::start()
{
    if(!started_)
	{
        gettimeofday(&stv_,NULL);
	 started_ = true;
	}
}

void  noDrift::sleep(time_t sec)
{
    struct timeval etv_; 
    suseconds_t elapsedtime = 0;
    time_t elapsed_sec=0,elapsed_usec=0,secs_to_go=0,usecs_to_go=0;
    static bool warning_ = true;
    if(sec < 1)
    {
      if(warning_ == true) 
      {
        std::cerr<<"wrong input sleep time " << sec << std::endl;
        warning_ = false;
      }
      sec = 1;
    } 
   if(started_)
   {           
    	gettimeofday(&etv_,NULL);
   	 //get the elapsed time
   
    	elapsedtime= (etv_.tv_sec - stv_.tv_sec)*USECS_IN_ONE_SEC + (etv_.tv_usec - stv_.tv_usec);
    	elapsed_sec=elapsedtime/USECS_IN_ONE_SEC;
    	elapsed_usec=elapsedtime%USECS_IN_ONE_SEC;		
	//elapsed_usec !=0 ,need to get 1 second from sec
	//transfer the 1 sec to USECS_IN_ONE_SEC,to caculate the usec_to go
	//elapsed_usec==0, don't need to compute the usec_to_go (=0) 
	if(elapsed_usec) 	
	{
		secs_to_go=max((time_t)0,sec-elapsed_sec-1); //not less than 0
		usecs_to_go=USECS_IN_ONE_SEC-elapsed_usec; //never be less than 0
	}
	else 
		secs_to_go=max((time_t)0,sec-elapsed_sec);
	::sleep(secs_to_go);
       ::usleep(usecs_to_go);    		
    	started_ = false;
    }
    else
	::sleep(sec);
		
}

