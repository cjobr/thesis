# thesis : Problem definition and Solution

## Problem definition
The idea of GPU sharing is putting multiple workloads on the same GPU to increase the GPU utilization. In GPU sharing, workloads not only share the compute resources also share the GPU memory. Due to the limited size of GPU memory and fast increasing memory footprints of workloads, the number of workloads can share a GPU is limited. This limitation becomes the bottleneck of GPU utilization. To allow more workloads to share a GPU, virtual memory mechanism is applied often since it can allow the total memory usage exceeding the physical memory capacity. However we observed that when virtual memory mechanism is applied and the total memory usage exceeds the physical memory size, the overhead of switching workloads will hurt the performance severely. Since if the workload wants to launch the kernel, it needs to copy its data back to GPU memory if its data is not present in GPU memory. This data transfer behavior is very expensive and will stalls GPU’s computaion unit. Prefetching necassary data to overlap the computation and data transfer time is a good solution which can hide the data transfer latency. And different scheduling results will effect how we can overlap the data transfer time and computation time. So, we think considering the data transfer behavior is also important in the scheduling of multiple workloads. A bad scheduling result may let overlapping become impossible.  

Example to Illustrate our observation(schedule and prefetch will affect the total time span)
![](https://i.imgur.com/CaQz4Oo.png)

* baseline (no prefetch) - 2375
![](https://i.imgur.com/5jQdloc.png)


* with prefetch - 2062.5
![](https://i.imgur.com/lAhzZcd.png)

* with prefetch and a better scheduling decision - 2005
![](https://i.imgur.com/huEZ84L.png)



## Solution
* some problems to solve the problem
    * how to know which tensor(memory size) the kernel want to access : current idea is ***assume that before each client start its computation, it will copy a fixed size of data to GPU memory***.(If prefetch all the data: lower-bound)
    * the format of input?    
* use genetic algorithm to get the optimized schedule.



![](https://i.imgur.com/jIcVjW6.png)
Implementation includes:
1. Initial population: population is a schedule plan, produce ***N*** populations in the begining.

![](https://i.imgur.com/cDkQ6y0.png)


2. Simulate populations: for each population, we can get execution time and memory swapping size from simulation. 
3. Select better candidates from initial population: we can select ***M*** better candidates based on a pre-defined function which is based on execution time of a population. There are some selection function(https://en.wikipedia.org/wiki/Selection_(genetic_algorithm)). We can choose a better selection function through experiments.
4. Crossover
random an crossover point, CR, to split the population. ex: 5
To create child P_C1, the crossover takes a
slice of P2 (P2[1 . . .CR] = [5,6,7,9,10]) to be the first part
of P_C1. The nodes not in the P_C1 are filled in according
to their order in P1.
![](https://i.imgur.com/HNMotG2.png =4000x160)



5. Mutation
Change a node's position in the list randomly as long as the result remains a topological ordering.


### Proposed algorithm
* definition
    * reward : the percentage of computaion during a period : computation/(computation + idle time)
    * penalty : the time that bring back the client(data transfer time)
    * weight : reward/penalty
    * window size : the period to calculate weight

* idea : considering two things in this problem
    * gpu sharing : idle time let GPU sharing improve GPU utilization.
    * the overlapping of computaion and data transfer : if we can hide data transfer under computation, can get maximum throughput.
* algorithm
    * periodically observe the clients in the job queue, calculate their weight
    * pick higher weight first : get long computation time and short data transfer time.
    * then pick lower weight : long data transfer time can be hided by long computation time of higher weight client.
    * pick higher weight : the short computation time of lower weight client is enough to hide short data transfer time of higher weight.
    * .........
 ```
 for client in the job_queue :
     calculate the reward and penalty
     calculate weight
     put client in candidate_list

 #if weight equals, smaller penalty one has bigger weight
 sort the client through their weight 
 
 while candidate_list is not empty :
     pick the client with the largest weight 
     pick the client with the smallest weight
 
 ```
* example to demonstrate the algorithm
    * profile to get each apps execution pattern, ex:
    ![](https://i.imgur.com/DGdkj5m.png)
    * calculate the weight for a window size:
    ![](https://i.imgur.com/oaKttPT.png)
    * sort to get the execution order, ex: D > C > A > B
    * execution order : D -> B -> C -> A
    ![](https://i.imgur.com/DYt6vIK.png)
    * move the time window and calculate weight for the next run
    ![](https://i.imgur.com/t6FxAiU.png)

    * .....






 


## Implemetation
* build a simulator to simulate.
* Components of the simulator
    * memory manager : manage the memory information, ex : record each client's memory location.
    * scheduler : periodically calculate the weight for each client, and decide the execution order.
    * execution time simulator : use profiled information(execution time of each operation) and the execution order from scheduler to simulate the execution.
## Compare with(and Related paper)
* vDNN
    * motivated by the layer-wise execution of network
    * built on cuDNN
    * swap intermidiate data(feature map and gradient)
    * use both eviction and prefetch
    * did not discuss the topic of GPU sharing
* SwapAdvisor 
    * also motivated by the characteristics of deep learning
    * based on MXNet
    * swap not only intermidiate data
    * also reschedule the order of operator
    * based on the schedule of operator, propose a swapping plan
    * use GA to find the optimal solution

* PipeSwitch
    * exploit the characteristics of deep learing, use the concept of pipeline
    * scenario is under GPU sharing, multiple deep learing workloads share a GPU
    * the goal is to using pipeline to hide the job switching overhead
    * propose a grouping algorithm(grouping layers of neural network)
    * implement a system intergrated with PyTorch
* Zico: Efficient GPU Memory Sharing for Concurrent DNN Training
    * exploit the characteristics of deep learning
    * idea is to overlap the peak memory usage and minimum memory usage of two workloads
    * limitation is only sharing two workloads
    * first profile the memory usage of each workload and the scheduler will decide the client’s delay time
    * implement Zico in tensorflow
* TicTac: Accelerating Distributed Deep Learning with Communication Scheduling
    * proposed scheduling algorithm of recv operation(parameter server based deep learning job)
    * the problem can be mapped to constrained job shop problem which is a NP-hard problem
    * define scheduling efficiency to measure its scheduling algorithm

