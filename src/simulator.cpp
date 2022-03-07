#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<queue>
#include<algorithm>
#include<sstream>

//basic setting of some parameters
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
  void update_weight()
  {
    double computation = 0; //computation time during the time window
    double idle = 0;        //idle time during the time window
    int i = this -> cur_position;
    double window = WINDOW_SIZE;
    double cur = this -> cur_time;
    while(window > 0 && i < time_.size())
    {
      if(this -> time_[i].first == 0)
      {
        if(this -> time_[i].second - cur < window)
        {
          window -= (this -> time_[i].second - cur);
          idle += (this -> time_[i].second - cur);
          //this -> cur_time = 0;
          cur = 0;
          i++;
        }
        else
        {
          idle += window;
          window = 0;
        }
      }
      else
      {
        if(this -> time_[i].second - cur < window)
        {
          window -= (this -> time_[i].second - cur);
          computation += (this -> time_[i].second - cur);
          i++;
        }
        else
        {
          computation += window;
          window = 0;
        }
      }
    }
    double reward = computation/(computation + idle);
    double penalty = this -> memory_transfer/BANDWIDTH;
    
    this -> weight_ = reward/penalty;
   

  }
  double get_weight()
  {
    return weight_;
  }
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

class MemoryManager {
  public:
    MemoryManager()
    {
      memory_used = 0;
      memory_aval = 16;
    }
    ~MemoryManager(){}
    void update_data_loacation(int client);
    void add_client_mem_info(double memory);
  private:
    vector<double> data_in_gpu; //record the size of data located in GPU memory for each client
    vector<double> data_in_host; //record the size of data located in host memory for each client
    double memory_used;
    double memory_aval;

};


//compare function of weight
bool compare(Client c1, Client c2)
{
  if(c1.get_weight() == c2.get_weight())return c1.memory_transfer < c2.memory_transfer;
  return c1.get_weight() < c2.get_weight();
}


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

int main(int argc, char *argv[])
{
  //input: open input file to read each client's information(time_ , memory, ....)
  char* testcase = argv[1];
  ifstream test(testcase);
  
  int num_client;
  string text;
  getline(test, text);
  num_client = std::stoi(text);
  vector<Client> apps;
  MemoryManager mm;
  queue<Client> job_queue;
  
  //initialize each client
  //cout<<num_client<<endl;
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
       cout<<kind<<" "<<t<<endl;
       temp -> time_.push_back(make_pair(kind, t));

    }
    //temp -> time_ = time_;
    temp -> cur_position = 0;
    temp -> cur_time = 0;
    temp -> update_weight();
    apps.push_back(*temp);
  }
  //app.close();
  test.close();
  //cout<<"file close"<<endl;
  //calculate maximal execution time
  double maximal = 0;
  for(int i = 0; i <apps.size(); i++)
  {
    for(auto time : apps[i].time_)maximal += time.second;
  }




  vector<Client> t(apps.begin(), apps.end());
  std::sort(t.begin(), t.end(), compare);
  job_queue.push(t[0]);
  job_queue.push(t[t.size() - 1]);
  double make_span = 0;
  double pre_make_span;
  double GPU_used = 0;
  make_span += (double)(job_queue.front().memory_transfer/BANDWIDTH);
  GPU_used += job_queue.front().memory_transfer;
  apps[job_queue.front().idx].memory_transfer = 0;

  //periodically calculate each client's weight, and get schedule order.
  int timestamp = 0;
  cout<<"cur time "<<make_span<<endl;
  //cout<<"start while loop"<<endl;
  while(!job_queue.empty())
  {
    
    //decide current client
    pre_make_span = make_span;
    timestamp++;
    Client cur = job_queue.front();
    //cout<<"current client id: "<<cur.idx<<endl;
    int cur_idx = cur.idx;
    int idle_cnt = 0;

    //before start simulate current client's execution, list each client's memory status and check the number of client which is idle
    for(int i = 0; i < apps.size(); i++)
    {
      cout<<"application: "<<i<<" memory_transfer: "<<apps[i].memory_transfer<<" memory_needed: "<<apps[i].memory_needed<<endl;
      if(apps[i].cur_position >= apps[i].time_.size() || apps[i].time_[apps[i].cur_position].first == 0)idle_cnt++;
    }
      
    job_queue.pop();

    //if current client is done, or current client is idle and there is an non-idle client, stop current client and push another client to the queue
    if(apps[cur_idx].cur_position >= apps[cur_idx].time_.size() || (apps[cur_idx].time_[apps[cur_idx].cur_position].first == 0 && idle_cnt != apps.size())) //|| apps[cur_idx].time_[apps[cur_idx].cur_position].first == 0
    {
      cout<<"current client: "<<cur_idx<<endl;
      cout<<"end execution"<<endl;
      cout<<"............................................"<<endl;
      t.clear();
      for(int i = 0; i < apps.size(); i++)
      {
        if(apps[i].cur_position < apps[i].time_.size() && i != cur_idx)
        {
          //apps.erase(apps.begin() + i);
          apps[i].update_weight();
          t.push_back(apps[i]);
        
        }
      }
      if(t.size() == 0)break;
      sort(t.begin(), t.end(), compare);
      if(t.size() > 1 && job_queue.size() == 1)
      {
        if(apps[t[0].idx].cur_position < apps[t[0].idx].time_.size())job_queue.push(t[0]);
        if(apps[t[t.size() - 1].idx].cur_position < apps[t[t.size() - 1].idx].time_.size())job_queue.push(t[t.size() - 1]);
     }
      else if(t.size() == 1 && job_queue.size() == 1)
      {
        if(apps[t[0].idx].cur_position < apps[t[0].idx].time_.size())job_queue.push(t[0]);
      }
       continue;
    }
    Client nxt = job_queue.front();
    apps[cur_idx].last_accessed = timestamp;
    //cout<<"gpu memory: "<<GPU_used<<"GB "<<endl;

    //update make_span
    if(GPU_used + apps[nxt.idx].memory_transfer <= MEMORY_CAPACITY)// if GPU memory is enough, directly prefetch
    {
      cout<<"enter enough gpu memory state"<<endl;

      //decide transfer time
      double transfer_time = apps[nxt.idx].memory_transfer/BANDWIDTH;
      cout<<"prefetch id: "<<nxt.idx<<"'s data"<<endl;
      GPU_used += apps[nxt.idx].memory_transfer;
      apps[nxt.idx].memory_transfer = 0;
      if(transfer_time == 0)transfer_time = STATIC_QUOTA;
      double computation_time = 0;

      //decide computation time
      for(int i = apps[cur_idx].cur_position; i < apps[cur_idx].time_.size() ;i++)
      {
        
        if(apps[cur_idx].time_[i].first == 0)
        {
          if(computation_time + apps[cur_idx].time_[i].second - apps[cur_idx].cur_time > transfer_time)
          {
            
            apps[cur_idx].cur_position = i;
            apps[cur_idx].cur_time = apps[cur_idx].cur_time + (transfer_time - computation_time);
            computation_time = transfer_time;
            break;
          }
          else if(computation_time + apps[cur_idx].time_[i].second - apps[cur_idx].cur_time == transfer_time)
          {
            computation_time += apps[cur_idx].time_[i].second - apps[cur_idx].cur_time;
            apps[cur_idx].cur_position = i + 1;
            apps[cur_idx].cur_time = 0;
            break;

          }
          else
          {
            computation_time += apps[cur_idx].time_[i].second - apps[cur_idx].cur_time;
            apps[cur_idx].cur_time = 0;
          }
        }
        else
        {
          if(computation_time + apps[cur_idx].time_[i].second >= transfer_time)
          {
            while(i < cur.time_.size() && cur.time_[i].first == 1)
            {
              computation_time += apps[cur_idx].time_[i].second;
              i++;
            }
            apps[cur_idx].cur_position = i;
            break;
          }
          else
          {
            computation_time += apps[cur_idx].time_[i].second;
          }
          
        }

      }
      cout<<"prefetch time: "<<transfer_time<<" computation time: "<<computation_time<<endl;

      //update makespan
      make_span += max(transfer_time, computation_time);
      //cout<<"makespan: "<<make_span<<endl;
      
    }
    else if(GPU_used + apps[nxt.idx].memory_transfer > MEMORY_CAPACITY && MEMORY_CAPACITY - apps[cur_idx].memory_needed >= apps[nxt.idx].memory_transfer)// if GPU memory is not enough, need to evict some data, then prefetch
    {
      cout<<"enter not enough gpu memory state"<<endl;
      double transfer_time = 0;

      //first evict neccesary victims to get enough memory space (include in transfer time)
      while(GPU_used + apps[nxt.idx].memory_transfer > MEMORY_CAPACITY)
      {
        //cout<<"start evict memory"<<endl;
        int victim = pick_victim(apps, cur_idx, nxt.idx);
        //cout<<"victim is : "<<victim<<endl;
        double evict_amount = min(apps[victim].memory_needed - apps[victim].memory_transfer, GPU_used + apps[nxt.idx].memory_transfer - MEMORY_CAPACITY);
        GPU_used -= evict_amount;
        //cout<<apps[victim].memory_needed<<" "<<apps[victim].memory_transfer<<endl;
        //cout<<"GPU memory after eviction: "<<GPU_used<<endl;
        transfer_time += evict_amount/BANDWIDTH;
        apps[victim].memory_transfer += evict_amount;
        cout<<"evict id: "<<victim<<"'s data"<<endl;
        
      }

      //then prefetch next client's data, and update transfer time
      transfer_time += apps[nxt.idx].memory_transfer/BANDWIDTH;
      cout<<"prefetch id: "<<nxt.idx<<"'s data"<<endl;
      GPU_used += apps[nxt.idx].memory_transfer;
      apps[nxt.idx].memory_transfer = 0;
      double computation_time = 0;

      //update computaion time
      for(int i = apps[cur_idx].cur_position; i < apps[cur_idx].time_.size() ;i++)
      {
        
        if(apps[cur_idx].time_[i].first == 0)
        {
          if(computation_time + apps[cur_idx].time_[i].second - apps[cur_idx].cur_time > transfer_time)
          {
            
            apps[cur_idx].cur_position = i;
            apps[cur_idx].cur_time = apps[cur_idx].cur_time + (transfer_time - computation_time);
            computation_time = transfer_time;
            break;
          }
          else if(computation_time + apps[cur_idx].time_[i].second - apps[cur_idx].cur_time == transfer_time)
          {
            computation_time += apps[cur_idx].time_[i].second - apps[cur_idx].cur_time;
            apps[cur_idx].cur_position = i + 1;
            apps[cur_idx].cur_time = 0;
            break;

          }
          else
          {
            computation_time += apps[cur_idx].time_[i].second - apps[cur_idx].cur_time;
            
            apps[cur_idx].cur_time = 0;
            if(i == apps[cur_idx].time_.size() - 1)apps[cur_idx].cur_position = i + 1;
          }
        }
        else
        {
          if(computation_time + apps[cur_idx].time_[i].second >= transfer_time)
          {
            while(i < cur.time_.size() && cur.time_[i].first == 1)
            {
              computation_time += apps[cur_idx].time_[i].second;
              i++;
            }
            apps[cur_idx].cur_position = i;
            break;
          }
          else
          {
            computation_time += apps[cur_idx].time_[i].second;
            if(i == apps[cur_idx].time_.size() - 1)apps[cur_idx].cur_position = i + 1;
          }
          
        }
      }
      cout<<"prefetch time: "<<transfer_time<<" computation time: "<<computation_time<<endl;

      //update makespan
      make_span += max(transfer_time, computation_time);


    }
    else if(MEMORY_CAPACITY - apps[cur_idx].memory_needed < apps[nxt.idx].memory_needed)// GPU memory is not enough even evict other clients's data, wait until current user stop computation
    {
      cout<<"enter not enough gpu memory state and need to wait until current client"<<endl;

      double transfer_time = 0;
      while(GPU_used + apps[nxt.idx].memory_transfer - apps[cur_idx].memory_needed > MEMORY_CAPACITY)
      {
        int victim = pick_victim(apps, cur_idx, nxt.idx);
        GPU_used -= (apps[victim].memory_needed - apps[victim].memory_transfer);
        apps[victim].memory_transfer = apps[victim].memory_needed;
        transfer_time += (apps[victim].memory_needed - apps[victim].memory_transfer)/BANDWIDTH;
        cout<<"evict id: "<<victim<<"'s data"<<endl;
      }
      double computation_time = 0;
      for(int i = apps[cur_idx].cur_position; i < apps[cur_idx].time_.size() ;i++)
      {
        
        if(apps[cur_idx].time_[i].first == 0)
        {
          if(computation_time + apps[cur_idx].time_[i].second - apps[cur_idx].cur_time > transfer_time)
          {
            
            apps[cur_idx].cur_position = i;
            apps[cur_idx].cur_time = apps[cur_idx].cur_time + (transfer_time - computation_time);
            computation_time = transfer_time;
            break;
          }
          else if(computation_time + apps[cur_idx].time_[i].second - apps[cur_idx].cur_time == transfer_time)
          {
            computation_time += apps[cur_idx].time_[i].second - apps[cur_idx].cur_time;
            apps[cur_idx].cur_position = i + 1;
            apps[cur_idx].cur_time = 0;
            break;

          }
          else
          {
            computation_time += apps[cur_idx].time_[i].second - apps[cur_idx].cur_time;
            apps[cur_idx].cur_time = 0;
          }
        }
        else
        {
          if(computation_time + apps[cur_idx].time_[i].second >= transfer_time)
          {
            while(i < cur.time_.size() && cur.time_[i].first == 1)
            {
              computation_time += apps[cur_idx].time_[i].second;
              i++;
            }
            apps[cur_idx].cur_position = i;
            break;
          }
          else
          {
            computation_time += apps[cur_idx].time_[i].second;
          }
          
        }
      }
      make_span += max(computation_time, transfer_time);
      make_span += (apps[cur_idx].memory_needed + apps[nxt.idx].memory_transfer)/BANDWIDTH;
      GPU_used = GPU_used - apps[cur_idx].memory_needed + apps[nxt.idx].memory_transfer;
      apps[nxt.idx].memory_transfer = 0;
      apps[cur_idx].memory_transfer = apps[cur_idx].memory_needed;
    }

    
    double duration_ = make_span - pre_make_span;
    cout<<"current application id: "<<cur.idx<<" ,quota: "<<duration_<<" memory used: "<<GPU_used<<endl;
    //cout<<"duraton : "<<duration<<endl;
    

    //update time_ position, only need to update clients which are idle
    for(int i = 0; i < apps.size(); i++)
    {
      double duration = duration_;
      //cout<<"i = "<<i<<" idx= "<<apps[i].idx<<endl;
      if(apps[i].idx == cur_idx)
      {
        cout<<"update application "<<apps[i].idx<<" "<<apps[i].cur_position<<" "<<apps[i].cur_time<<endl;
        //cout<<"current client"<<endl;
        continue;
      }
      if(apps[i].time_[apps[i].cur_position].first == 1)
      {
        cout<<"update application "<<apps[i].idx<<" "<<apps[i].cur_position<<" "<<apps[i].cur_time<<endl;
        //cout<<"kernel"<<endl;
        continue;
      }
      while(apps[i].cur_position < apps[i].time_.size() && apps[i].time_[apps[i].cur_position].first == 0 && duration > 0)
      {
        if(apps[i].time_[apps[i].cur_position].second - apps[i].cur_time <= duration)
        {
          duration -= (apps[i].time_[apps[i].cur_position].second - apps[i].cur_time);
          apps[i].cur_position++;
          apps[i].cur_time = 0;
        }
        else
        {
          apps[i].cur_time += duration;
          duration = 0;
        }

      }
      cout<<"update application "<<apps[i].idx<<" "<<apps[i].cur_position<<" "<<apps[i].cur_time<<endl;

    }

    t.clear();


    //compute each client's weight except current client
    
    for(int i = 0; i < apps.size(); i++)
    {
      //cout<<apps[i].cur_position<<endl;
      if(apps[i].cur_position < apps[i].time_.size() && i != cur_idx)
      {
        //apps.erase(apps.begin() + i);
        apps[i].update_weight();
        t.push_back(apps[i]);
        
      }
      if(apps[i].cur_position >= apps[i].time_.size())
      {
        if(apps[i].finish == 0)
        {
          GPU_used -= (apps[i].memory_needed - apps[i].memory_transfer);
          apps[i].finish = 1;
          apps[i].memory_transfer = 0;
        }

        
        
      }
      
    }
    //cout<<t.size()<<endl;
    if(t.size() == 0)
    {
      for(int i = 0; i < apps.size(); i++)
      {
        if(apps[i].cur_position < apps[i].time_.size() - 1)
        {
          t.push_back(apps[i]);
        }
      }
    }
    if(t.size() == 0)
    {
      cout<<"end simulation"<<endl;
      break;
    }
    sort(t.begin(), t.end(), compare);

    //add clients to job queue when the size of queue is equal to 1 or less
    if(t.size() > 1 && job_queue.size() <= 1)
    {
      if(apps[t[0].idx].cur_position < apps[t[0].idx].time_.size())job_queue.push(t[0]);
      if(apps[t[t.size() - 1].idx].cur_position < apps[t[t.size() - 1].idx].time_.size())job_queue.push(t[t.size() - 1]);
    }
    else if(t.size() == 1 && job_queue.size() <= 1)
    {
      if(apps[t[0].idx].cur_position < apps[t[0].idx].time_.size())job_queue.push(t[0]);
    }
    
    
    cout<<"............................................"<<endl;
  }
  cout<<"total time: "<<make_span<<endl;
  cout<<"expected maximal time: "<<maximal<<endl;

}