mpirun -ha -spawn -TCP -e MPI_TEST_DELAY=200 -e MPI_TMPDIR=/home/chultgren -hostfile=./cluster.conf ./monitor COLD

