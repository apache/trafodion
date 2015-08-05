///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <sys/time.h>

/* max message size to test */
#define MAXCOUNT (1048576 * 8)

/* number of tests per message size */
#define NUMTESTS 100

int main(int argc, char **argv)
{
  int i, j, rank, size;
  int to, from, tag;
  int count;
  char *message;
  MPI_Status status;
  double total_time,base_time,current_time,max_time,min_time,avg_time;

  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  message = (char*) malloc (MAXCOUNT * sizeof(char));

  from = 1 - rank;
  to = 1 - rank;
  tag = 666;

  if (rank == 0) 
  {
    for (count = 1, j = 0; count <= MAXCOUNT; count*=2, j++) 
    {
      fprintf(stderr,"%10d",count);
      MPI_Barrier(MPI_COMM_WORLD);
      total_time = 0;
      max_time = 0;
      min_time = 99999999;
      for (i = 0; i < NUMTESTS; i++) 
      {
	    fprintf(stderr,".");
        base_time = MPI_Wtime();
	    MPI_Send(message, count, MPI_CHAR, to, tag, MPI_COMM_WORLD);
        MPI_Recv(message, count, MPI_CHAR, /*from*/MPI_ANY_SOURCE, /*tag*/MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        current_time = ( MPI_Wtime()-base_time );
        total_time += current_time;
        if( current_time > max_time ) max_time = current_time;
        if( current_time < min_time ) min_time = current_time;
      }
      fprintf(stderr,"\n");
      avg_time = total_time/NUMTESTS;
      printf("Total=%f, Min=%f, Max=%f, Avg=%f, MB/sec.=%f\n", 
             total_time, min_time, max_time, avg_time,
             ((NUMTESTS*count*8.0)*(1/total_time))/1048576.0 );
    }
  } 
  else 
  {
    if (rank == 1) 
    {
      for (count = 1; count <= MAXCOUNT; count*=2) 
      {
        MPI_Barrier(MPI_COMM_WORLD);
        for (i = 0; i < NUMTESTS; i++) 
        {
		  MPI_Recv(message, count, MPI_CHAR, from, tag, MPI_COMM_WORLD, &status);
		  MPI_Send(message, count, MPI_CHAR, to, tag, MPI_COMM_WORLD);
        }
      }
    } 
    else 
    {
      /* no point in running this on more than two nodes... */
      fprintf(stderr,"notice: pingpong requires only two nodes to run on.\n");
    }
  }


  /* clean up */
  fflush(stdout);

  free(message);

  MPI_Finalize();
  return 0;
}
