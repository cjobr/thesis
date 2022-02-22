//local solver
#include <cmath>
#include "localsolver.h"
#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<queue>
#include<algorithm>
#include<sstream>
#include <unordered_map>



using namespace localsolver;


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

double calculateComputation(vector<int> p, int range, int indexOfP)
{
  double computation_time = 0;
  int cur = ClientOfKernel(p[indexOfP]);
  for(int i = indexOfKernel(p[indexOfP]); i < apps[cur].time_.size(); i++)
  {
    computation_time += apps[cur].time_[i].second;
  }
  return computation_time;

}
class LSmakespan : public LSExternalFunction<lsdouble> {
public:
    lsdouble call(const LSExternalArgumentValues& argumentValues) {
        LSCollection permutation = argumentValues.getCollectionValue(0);
        vector<Client> temp = apps;
        int size = permutation.count();
        std::vector<int> p;
        std::vector<int> c;
        std::vector<int> idx;
        int first_client;
        for(int i = 0; i < size; i++)
        {
          p.push_back(permutation.get(i));
          c.push_back(ClientOfKernel(p[i]));
          idx.push_back(indexOfKernel(p[i]));
        }

        int indexOfP = 0;
        double make_span = 0;
       
        int Isend = 0;
        while(indexOfP < p.size())
        {
           if(indexOfP == 0)
           {
              first_client = c[0];
              make_span += temp[first_client].memory_transfer/BANDWIDTH;
              GPU_used += temp[first_client].memory_transfer;
              temp[first_client].memory_transfer = 0;
           }
           int cur_client = ClientOfKernel(p[indexOfP]);
           //cout<<p[indexOfP]<<endl;
           int range = findRangeOfClient(p, indexOfP);
           int next_client;
           if(range + 1 < p.size())next_client = ClientOfKernel(p[range + 1]);
           else Isend = 1;

           double transfer_time = 0;
           double computation_time = calculateComputation(p, range, indexOfP);
           if(Isend)
           {
             make_span += computation_time;
             break;
           }
           if(GPU_used + temp[next_client].memory_transfer <= MEMORY_CAPACITY)
           {

           }
           else if(GPU_used + temp[next_client].memory_transfer > MEMORY_CAPACITY && MEMORY_CAPACITY - temp[cur_client].memory_needed >= temp[next_client].memory_transfer)
           {

           }
           indexOfP++;
        }
        return make_span;
        
    }
};


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
  
  LocalSolver ls;
  LSModel m = ls.getModel();
  LSParam param = ls.getParam();
  param.setNbThreads(1);
  LSExpression kernels = m.listVar(count);
  m.constraint(m.count(kernels) == count);
  for(int i = 0; i < index.size(); i++)
  {
    for(int j = 1; j < index[i].size(); j++)
    {
     m.constraint(m.indexOf(kernels, index[i][j - 1]) < m.indexOf(kernels, index[i][j]));
    }
    cout<<endl;
  }
  LSmakespan makespanCode;
// Step 2: Turn the external code into an LSExpression
  LSExpression makespan = m.createExternalFunction(&makespanCode);
 
// Step 3: Call the function
  m.minimize(makespan(kernels));
  m.close();
  ls.solve();
  //ls.getSolution();
}