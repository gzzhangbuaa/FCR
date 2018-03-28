# ***___FCR___***
A library implemented in the user level to provide supporting for failures distinguishing  
(face to large-scale MPI applications)   
## ***___APPENDIX___***    
### **___A. Abstract___**     
This artifact contains the code to build the FCR library and the information needed to launch some experiments in the paper “A Lightweight and Flexible Tool for Distinguishing between Hardware Malfunctions and Program Bugs in Debugging Large-scale Programs”. We explain how to compile and run the modified benchmarks used in Section IV.
### **___B. Description___**
#### **1)	Check-list (artifact meta information):**    
**•	Program:** C and MPI code, C library    
**•	Compilation:** mpiicc (icc-14.0.2) with the -O2 flag    
**•	Binary:** HPL, HPCG, HPCC executables   
**•	Data Set:** default input of the three benchmarks but need small modification according to the demand for running at scale     
**•	OS environment:** Red Hat Enterprise Linux Server release 6.5 (Santiago) kernel version-2.6.32, intel-MPI-5.0.2     
**•	Hardware:** Tianhe-2 supercomputer, each node has  two Intel Xeon E5-2692 v2 CPU with 24 cores and 64GB memory. We use up to 256 nodes to complete the overhead evaluation in Section IV.    
**•	Output:** 	execution time, status log    
**•	Experiment workflow:** build FCR library, modify benchmarks code with APIs provided by FCR, submit a job script to the PBS batch system, wait for the job to be scheduled, and then collect the results.      
**•	Experiment customization:** modify the benchmarks by adding FCR library calls and simulating hardware malfunctions or software bugs, modify configure file for running at different scales    
**•	Publicly available?:** Yes.   

#### **2)	How delivered:**    
      HPL, HPCG and HPCC are open- source benchmarks, you can get them in the following URLs separately: 
      http://www.netlib.org/benchmark/hpl/    
      http://www.hpcg-benchmark.org/        
      http://icl.cs.utk.edu/hpcc/   
#### **3)  Hardware dependencies:**     
We used Tianhe-2 supercomputer for performance evaluation and functional verification.      
#### **4)  Software dependencies:**
HPL depends on BLAS or Intel MKL. We used the existing HPL on Tianhe-2 for our experiments in Section IV, which uses Intel MKL as its math library.
#### **5)  Datasets:**
The performance evaluation requires running the target application with and without FCR under various running scales. It’s necessary to adjust the corresponding parameters in the input files including problem size and running scales, as well as the content in submit script.

      
      

      







