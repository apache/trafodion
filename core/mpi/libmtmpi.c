// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

#include "mpi.h"

#include <stdlib.h>
#include <string.h>
#include "SCMVersHelp.h"

// component version for libraries
#define VERS_CV_MAJ 1
#define VERS_CV_MIN 0
#define VERS_CV_UPD 1

VERS_LIB(mpi_libmtmpi)

static int initialized = 0;
static int finalized = 0;

int
MPI_Init(int *argc, char ***argv)
{
	if (initialized) return MPI_ERR_INTERN;
	initialized = 1;
	finalized = 0;
	return MPI_SUCCESS;
}

int MPI_Abort(MPI_Comm comm, int errorcode)
{
	return MPI_ERR_INTERN;
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
	return MPI_ERR_INTERN;
}

int
MPI_Barrier(MPI_Comm comm)
{
	return MPI_ERR_INTERN;
}

int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root,
               MPI_Comm comm )
{
	return MPI_ERR_INTERN;
}

int
MPI_Cancel(MPI_Request *request)
{
	return MPI_ERR_INTERN;
}

int
MPI_Close_port(const char *port_name)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)

{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_disconnect(MPI_Comm *comm)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_dup(MPI_Comm comm, MPI_Comm *ncomm)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_free(MPI_Comm *comm)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_get_parent(MPI_Comm *comm)
{
	return MPI_ERR_INTERN;
}

int MPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_rank(MPI_Comm comm, int *rank)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_remote_size(MPI_Comm comm, int *size)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_size(MPI_Comm comm, int *size)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_spawn_multiple(int count, char *array_of_commands[],
                        char* *array_of_argv[], const int array_of_maxprocs[],
                        const MPI_Info array_of_info[], int root, MPI_Comm comm,
                        MPI_Comm *intercomm, int array_of_errcodes[])
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
	return MPI_ERR_INTERN;
}

int
MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
	return MPI_SUCCESS;
}

int
MPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
	return MPI_ERR_INTERN;
}

int
MPI_Errhandler_create(MPI_Handler_function *function,
                      MPI_Errhandler * errhandler)
{
	return MPI_ERR_INTERN;
}

int MPI_Errhandler_free(MPI_Errhandler *errhandler)
{
	return MPI_ERR_INTERN;
}

int
MPI_Error_class(int errcode, int *errclass)
{
	return MPI_ERR_INTERN;
}

int
MPI_Error_string(int errcode, char *string, int *resultlen)
{
	return MPI_ERR_INTERN;
}

int
MPI_Finalize(void)
{
	if (!initialized) return MPI_ERR_INTERN;
	initialized = 0;
	finalized = 1;
	return MPI_SUCCESS;
}

int
MPI_Finalized(int *flag)
{
	*flag = finalized;
	return MPI_SUCCESS;
}

int
MPI_Get_count(const MPI_Status *status, MPI_Datatype dtype, int *count)
{
	return MPI_ERR_INTERN;
}

int
MPI_Get_processor_name(char *name, int *resultlen)
{
	return MPI_ERR_INTERN;
}

int MPI_Group_free(MPI_Group *group)
{
	return MPI_ERR_INTERN;
}

int MPI_Group_size(MPI_Group group, int *size)
{
	return MPI_ERR_INTERN;
}

int
MPI_Info_create(MPI_Info *info)
{
	return MPI_ERR_INTERN;
}

int
MPI_Info_free(MPI_Info *info)
{
	return MPI_ERR_INTERN;
}

int
MPI_Info_set(MPI_Info info, const char *key, const char *val)
{
	return MPI_ERR_INTERN;
}

int
MPI_Initialized(int *flag)
{
	*flag = initialized;
	return MPI_SUCCESS;
}

int
MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
	if (initialized) return MPI_ERR_INTERN;
	initialized = 1;
	finalized = 0;
	return MPI_SUCCESS;
}

int
MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
                     int remote_leader, int tag, MPI_Comm *newintercomm)
{
	return MPI_ERR_INTERN;
}

int
MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
	return MPI_ERR_INTERN;
}

int
MPI_Irecv(void *buff, int count, MPI_Datatype dtype, int source, int tag,
          MPI_Comm comm, MPI_Request *req)
{
	return MPI_ERR_INTERN;
}

int
MPI_Isend(const void *buff, int count, MPI_Datatype dtype, int dest, int tag,
          MPI_Comm comm, MPI_Request *req)
{
	return MPI_ERR_INTERN;
}

int
MPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name)
{
	return MPI_ERR_INTERN;
}

int
MPI_Open_port(MPI_Info info, char *port_name)
{
	*port_name = 0;
	return MPI_SUCCESS;
}

int
MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
	return MPI_ERR_INTERN;
}

int
MPI_Publish_name(const char *service_name, MPI_Info info, const char *port_name)
{
	return MPI_ERR_INTERN;
}

int
MPI_Recv(void *buff, int count, MPI_Datatype dtype, int source, int tag,
         MPI_Comm comm, MPI_Status *status)
{
	return MPI_ERR_INTERN;
}

int
MPI_Request_free(MPI_Request *req)
{
	return MPI_ERR_INTERN;
}

int
MPI_Send(const void *buff, int count, MPI_Datatype dtype, int dest, int tag,
         MPI_Comm comm)
{
	return MPI_ERR_INTERN;
}

int
MPI_Sendrecv(const void *sbuff, int scount, MPI_Datatype sdtype, int src,
             int stag,
             void *rbuff, int rcount, MPI_Datatype rdtype, int dst, int rtag,
             MPI_Comm comm, MPI_Status *status)
{
	return MPI_ERR_INTERN;
}

int
MPI_Test(MPI_Request *req, int *flag, MPI_Status *status)
{
	return MPI_ERR_INTERN;
}

int
MPI_Testany(int count, MPI_Request array_of_requests[], int *index,
               int *flag, MPI_Status *status)
{
	return MPI_ERR_INTERN;
}

int
MPI_Testsome(int incount, MPI_Request *reqs, int *outcount, int *ind,
             MPI_Status *stats)
{
	return MPI_ERR_INTERN;
}

int
MPI_Wait(MPI_Request *req, MPI_Status *status)
{
	return MPI_ERR_INTERN;
}

int
MPI_Waitall(int nreqs, MPI_Request *reqs, MPI_Status *stats)
{
	return MPI_ERR_INTERN;
}

int
MPI_Waitany(int nreqs, MPI_Request *reqs, int *ind, MPI_Status *stats)
{
	return MPI_ERR_INTERN;
}

int
MPI_Waitsome(int nreqs, MPI_Request *reqs, int *outcount, int *ind,
             MPI_Status *stats)
{
	return MPI_ERR_INTERN;
}

double
MPI_Wtick(void)
{
	return MPI_ERR_INTERN;
}

double
MPI_Wtime(void)
{
	return MPI_ERR_INTERN;
}
