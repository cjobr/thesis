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
class LSmakespan : public LSExternalFunction<lsdouble> {
public:
    lsdouble call(const LSExternalArgumentValues& argumentValues) {
        LSCollection permutation = argumentValues.getCollectionValue(0);
        int size = permutation.count();
        std::vector<int> p;
        for(int i = 0; i < size; i++)
        {
          p.push_back(permutation[i]);
          //std::cout<<p[i]<<" ";
        }
        lsdouble t;

        return  0.0;
    }
};

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
    temp -> memory_transfer = temp -> memory_needed;
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
  LSExpression kernels = m.listVar(count);
  m.constraint(m.count(kernels) == count);
  for(int i = 0; i < index.size(); i++)
  {
    for(int j = 1; j < index[i].size(); j++)
    {
     m.constraint(m.indexOf(kernels, index[i][j - 1]) < m.indexOf(kernels, index[i][j]));
    }
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