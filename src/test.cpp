//local solver
#include <cmath>
//#include "localsolver.h"
#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<queue>
#include<algorithm>
#include<sstream>
#include <unordered_map>



//using namespace localsolver;


#define BANDWIDTH 0.01 // (GB/ms)
#define WINDOW_SIZE 500.0 //ms
#define MEMORY_CAPACITY 16.0 //GB
#define STATIC_QUOTA 250.0 //ms 
using namespace std;

//the class of each client
class Client {
 public:
  Client()
  {
    weight_ = 0;
  }
  ~Client(){}

  //calculate penalty
  
  vector<pair<int, double>> time_; //0: idle, 1: kernel

  //the memory in GPU memory = memory_needed - memory_transfer
  double memory_needed;  //the total memory which this application needs
  double memory_transfer; //the memory which needs to be transfered
  int cur_position; // index of time_
  double cur_time;  //the current time in idle state
  int last_accessed; //the timestamp to denote the last time which memory was accessed
  int idx; 
  int finish = 0;
  

 private:
  double weight_;  // sheduling priority (reward/penalty)
  
};

//decide which client to be evicted when memory is not enough, basically LRU
int pick_victim(vector<Client> &apps, int cur_idx, int nxt_idx)
{
  int idx;
  
  for(int i = 0; i < apps.size(); i++)
  {
    if(apps[i].memory_needed == apps[i].memory_transfer || i == cur_idx || i == nxt_idx)continue;
    idx = i;

  }
  for(int i = 0; i < apps.size(); i++)
  {
    if(apps[i].memory_needed == apps[i].memory_transfer || i == cur_idx || i == nxt_idx)continue;
    if(apps[i].last_accessed < apps[idx].last_accessed)
    {
      idx = i;
    }
  }
  return idx;
}
vector<Client> apps;
unordered_map<int, vector<int>> map;
double GPU_used = 0;
int ClientOfKernel(int number)
{
  if(!map.count(number))
  {
    cout<<"not found!"<<endl;
    return 0;
  }
  vector<int> res = map[number];
  if(res.size() > 0)return res[0];
  else return 0;
}
int indexOfKernel(int number)
{
  return map[number][1];
}

int findRangeOfClient(vector<int> p, int indexOfP)
{
  int i;
  for(i = indexOfP; i < p.size() - 1; i++)
  {
    if(ClientOfKernel(p[i]) != ClientOfKernel(p[i+ 1]))return i;
  }
  return i;
  
}

double calculateComputation(vector<int> p, int range, int indexOfP, vector<Client>& temp)
{
  double computation_time = 0;
  int cur = ClientOfKernel(p[indexOfP]);
  int start = indexOfKernel(p[indexOfP]);
  while(start != temp[cur].cur_position)
  {
    computation_time += temp[cur].time_[temp[cur].cur_position].second - temp[cur].cur_time;
    temp[cur].cur_position++;
    temp[cur].cur_time = 0;
  }
  for(int i = start; i <= indexOfKernel(p[range]); i++)
  {
    computation_time += temp[cur].time_[i].second;
  }
  temp[cur].cur_position = indexOfKernel(p[range]) + 1;
  return computation_time;

}

double compute_makespan() {
        //LSCollection permutation = argumentValues.getCollectionValue(0);
        vector<Client> temp = apps;
        
        std::vector<int> p = {0,1,2,3,4,5,6,21,22,23,24,25,26,27, 42,43,44,45,46,47,48, 7,8,9,10,11,12,13, 28,29,30,31,32,33,34, 49,50,51,52,53,54,55, 14,15,16,17,18,19,20, 35,36,37,38,39,40,41, 56,57,58,59,60,61,62};
        cout<<p.size()<<endl;
        int size = p.size();
        std::vector<int> c;
        std::vector<int> idx;
        int first_client;
        for(int i = 0; i < size; i++)
        {
          c.push_back(ClientOfKernel(p[i]));
          idx.push_back(indexOfKernel(p[i]));
        }
        int timestamp = 0;
        int indexOfP = 0;
        double make_span = 0;
        double last_make_span;
        int Isend = 0;
        while(indexOfP < p.size())
        {
           timestamp++;
           last_make_span = make_span;
           if(indexOfP == 0)
           {
              
              first_client = c[0];
              make_span += temp[first_client].memory_transfer/BANDWIDTH;
              GPU_used += temp[first_client].memory_transfer;
              temp[first_client].memory_transfer = 0;
              double duration = make_span - last_make_span;
              cout<<"transfer first client's data transfer time: "<< duration<<endl;
           }
           int cur_client = ClientOfKernel(p[indexOfP]);
           cout<<"current client: "<<cur_client<<endl;
           //cout<<p[indexOfP]<<endl;
           int range = findRangeOfClient(p, indexOfP);
          // cout<<"range = "<<range<<endl;
           int next_client;
           if(range + 1 < p.size())next_client = ClientOfKernel(p[range + 1]);
           else Isend = 1;

           double transfer_time = 0;
           double computation_time = calculateComputation(p, range, indexOfP, temp);
           
           temp[cur_client].last_accessed = timestamp;
           if(Isend)
           {
             
             make_span += computation_time;
             double duration = make_span - last_make_span;
             cout<<"duration :"<< duration<<endl;
             break;
           }
           if(GPU_used + temp[next_client].memory_transfer <= MEMORY_CAPACITY)
           {
               cout<<"enter enough memory state"<<endl;
               cout<<"prefetch client "<<next_client<<"'s data"<<endl;
               double transfer_time = temp[next_client].memory_transfer/BANDWIDTH;
               GPU_used += temp[next_client].memory_transfer;
               temp[next_client].memory_transfer = 0;
               
               int cur_position = temp[cur_client].cur_position;
               if(computation_time < transfer_time && temp[cur_client].time_[cur_position].first == 0)
               {
                    if(computation_time + temp[cur_client].time_[cur_position].second - temp[cur_client].cur_time > transfer_time)
                    {
            
                        temp[cur_client].cur_position = cur_position;
                        temp[cur_client].cur_time = temp[cur_client].cur_time + (transfer_time - computation_time);
                        computation_time = transfer_time;
                        
                    }
                    else if(computation_time + temp[cur_client].time_[cur_position].second - temp[cur_client].cur_time == transfer_time)
                    {
                        computation_time += temp[cur_client].time_[cur_position].second - temp[cur_client].cur_time;
                        temp[cur_client].cur_position = cur_position + 1;
                        temp[cur_client].cur_time = 0;
                        

                    }
                    else
                    {
                        computation_time += temp[cur_client].time_[cur_position].second - temp[cur_client].cur_time;
                        temp[cur_client].cur_time = 0;
                        temp[cur_client].cur_position = cur_position + 1;
                    }
               }
               cout<<"computation time: "<<computation_time<<endl;
               cout<<"transfer time: "<<transfer_time<<endl;
               make_span += max(transfer_time, computation_time);
               cout<<make_span<<endl;
           }
           else if(GPU_used + temp[next_client].memory_transfer > MEMORY_CAPACITY && MEMORY_CAPACITY - temp[cur_client].memory_needed >= temp[next_client].memory_transfer)
           {
               cout<<"enter not enough memory state"<<endl;
               double transfer_time = 0;

                //first evict neccesary victims to get enough memory space (include in transfer time)
                while(GPU_used + temp[next_client].memory_transfer > MEMORY_CAPACITY)
                {
                    //cout<<"start evict memory"<<endl;
                    int victim = pick_victim(temp, cur_client, next_client);
                    //cout<<"victim is : "<<victim<<endl;
                    double evict_amount = min(temp[victim].memory_needed - temp[victim].memory_transfer, GPU_used + temp[next_client].memory_transfer - MEMORY_CAPACITY);
                    GPU_used -= evict_amount;
                    //cout<<apps[victim].memory_needed<<" "<<apps[victim].memory_transfer<<endl;
                    //cout<<"GPU memory after eviction: "<<GPU_used<<endl;
                    transfer_time += evict_amount/BANDWIDTH;
                    temp[victim].memory_transfer += evict_amount;
                    cout<<"evict id: "<<victim<<"'s data"<<endl;
        
                }

                //then prefetch next client's data, and update transfer time
                transfer_time += temp[next_client].memory_transfer/BANDWIDTH;
                cout<<"prefetch id: "<<next_client<<"'s data"<<endl;
                GPU_used += temp[next_client].memory_transfer;
                temp[next_client].memory_transfer = 0;
                int cur_position = temp[cur_client].cur_position;
               if(computation_time < transfer_time && temp[cur_client].time_[cur_position].first == 0)
               {
                    if(computation_time + temp[cur_client].time_[cur_position].second - temp[cur_client].cur_time > transfer_time)
                    {
            
                        temp[cur_client].cur_position = cur_position;
                        temp[cur_client].cur_time = temp[cur_client].cur_time + (transfer_time - computation_time);
                        computation_time = transfer_time;
                        
                    }
                    else if(computation_time + temp[cur_client].time_[cur_position].second - temp[cur_client].cur_time == transfer_time)
                    {
                        computation_time += temp[cur_client].time_[cur_position].second - temp[cur_client].cur_time;
                        temp[cur_client].cur_position = cur_position + 1;
                        temp[cur_client].cur_time = 0;
                        

                    }
                    else
                    {
                        computation_time += temp[cur_client].time_[cur_position].second - temp[cur_client].cur_time;
                        temp[cur_client].cur_time = 0;
                        temp[cur_client].cur_position = cur_position + 1;
                    }
               }
               cout<<"computation time: "<<computation_time<<endl;
               cout<<"transfer time: "<<transfer_time<<endl;
               make_span += max(transfer_time, computation_time);
               

                
           }
           double duration_ = make_span - last_make_span;
           cout<<"duration: "<<duration_<<endl;
           for(int i = 0; i < temp.size(); i++)
           {
                double duration = duration_;
                if(temp[i].idx == cur_client)
                {
                    cout<<"update application "<<temp[i].idx<<" "<<temp[i].cur_position<<" "<<temp[i].cur_time<<endl;
                    //cout<<"current client"<<endl;
                    continue;
                }
                if(temp[i].time_[temp[i].cur_position].first == 1)
                {
                    cout<<"update application "<<temp[i].idx<<" "<<temp[i].cur_position<<" "<<temp[i].cur_time<<endl;
                    //cout<<"kernel"<<endl;
                    continue;
                }
                while(temp[i].cur_position < temp[i].time_.size() && temp[i].time_[temp[i].cur_position].first == 0 && duration > 0)
                {
                    if(temp[i].time_[temp[i].cur_position].second - temp[i].cur_time <= duration)
                    {
                        duration -= (temp[i].time_[temp[i].cur_position].second - temp[i].cur_time);
                        temp[i].cur_position++;
                        temp[i].cur_time = 0;
                     }
                    else
                    {
                        temp[i].cur_time += duration;
                        duration = 0;
                    }

                }
                cout<<"update application "<<temp[i].idx<<" "<<temp[i].cur_position<<" "<<temp[i].cur_time<<endl;

           }
           cout<<"current makespan :"<<make_span<<endl;
           cout<<"........................................"<<endl;
           indexOfP = range + 1;
        }
        return make_span;
        
}

int main(int argc, char *argv[])
{
    //input: open input file to read each client's information(time_ , memory, ....)
  char* testcase = argv[1];
  ifstream test(testcase);
  
  int num_client;
  string text;
  getline(test, text);
  num_client = std::stoi(text);
  
  queue<Client> job_queue;
  
  //initialize each client
  cout<<num_client<<endl;
  for(int i = 0; i < num_client; i++)
  {
    Client* temp = new Client();
    getline(test, text);
    cout<<"file name: "<<text<<endl;
    ifstream app(text);
    getline(app, text);
    temp -> memory_needed = std::stod(text);
    cout<<temp -> memory_needed<<endl;
    temp -> memory_transfer = temp -> memory_needed;
    cout<<temp -> memory_transfer<<endl;
    temp -> idx = i;
    string str;
    while(getline(app, text, '\n'))
    {
       
       std::istringstream input;
       input.str(text);
       vector<string> s;
       for(std::string line ; std::getline(input, line, ' '); )
       {
         s.push_back(line);

       }
       int kind = std::stoi(s[0]);
       double t = std::stod(s[1]);
       //cout<<kind<<" "<<t<<endl;
       temp -> time_.push_back(make_pair(kind, t));

    }
    //temp -> time_ = time_;
    temp -> cur_position = 0;
    temp -> cur_time = 0;
    apps.push_back(*temp);
  }
  //app.close();
  test.close();


  int count = 0;
  vector<vector<int>> index;
  
  //unordered_map<vector<int>, int> index_to_count;
  for(int i = 0; i < apps.size(); i++)
  {
    vector<int> temp;
    for(int j = 0; j < apps[i].time_.size(); j++)
    {
      if(apps[i].time_[j].first == 1)
      {
        map[count] = {i, j};
        temp.push_back(count);
        count++;
      }
    }
    index.push_back(temp);

  }
  double make_span = compute_makespan();
  cout<<"makespan = "<<make_span<<endl;
  
  //ls.getSolution();
}