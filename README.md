# FCR
a library implemented in the user level to provide supporting for failures distinguishing 
(face to large-scale MPI applications)
Make sure your environment has installed the MPI implementation correctly.
cd /src
./make.sh
A static library named libpdi.a is generated in the /lib path
pdi.h exists in /include fold
use libpdi.a and pdi.h to complete the program modification.

The three most important APIs are as follows
#insert after the MPI call MPI_init()
PDI_Init();
#insert before the MPI call MPI_Finalize()
PDI_Finalize();
#insert in the wrappers of MPI communication calls such as HPL_Send(), HPL_Recv, etc.
#Knock-Knock to tell the daemon process we (worker processes) are alive and communicating.
PDI_Knock();

We implemented simulation for hardware malfunctions and software bugs simply.
The corresponding API is PDI_FailureTest(int nodeID, int type),
nodeID represents the node ID you specified where the failure occurs.
through the type parameter to determine whether the failure you expected is caused
by hardware or software errors.


APPENDIX
==========
