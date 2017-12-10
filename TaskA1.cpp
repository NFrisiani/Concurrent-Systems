/*

- Request use of the ADC by calling the requestADC() method of the ADC object. This call
passes an integer argument representing the channel to be sampled. The ADC has several channels
(initially six), each represented by an instance of AdcInputChannel. Assume that each thread
uses a unique AdcInputChannel. The thread should print out its identity when the call is
successful.

- Request a sample from the ADC by calling sampleADC(). This returns a double value on the
basis of the channel that was selected by the requestADC() call. To start with, simply make
the sampleADC() method return a value that is twice the channel number passed in the call to
requestADC().

- Print out its identity together with the sample value obtained from the ADC.

- Call releaseADC() to enable other threads to use the ADC, printing out its identity once more.

- Delay for a random time between 0.1 and 0.5 second.

*/


//File inclusions
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <random>
#include <string>
using namespace std;

//Function declaration
int search();
//void run(ADC& theADC, int id);

//Global Variables
int const MAX_NUM_OF_CHAN = 6;
int const MAX_NUM_OF_THREADS = 6;
int const DATA_BLOCK_SIZE = 20;

std::mutex mutexADC;

std::mutex mtx;
std::condition_variable cond_var;

std::mutex mu;
std::map<std::thread::id, int> threadIDs;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(100,500);



class AdcInputChannel
{
  public:
    AdcInputChannel(int d) : currentSample(d) {}

    double getCurrentSample()  //used to request a sample from the sample channel
    {
      return 2*currentSample;
    }

  private:
    int currentSample;
};



class Lock
{
  public:
    Lock() : open(false) {}

    void lock() //returns a flag to indicate when a thread should be blocked
    {
      open = 1;
      mutexADC.lock();
    }

    void unlock()
    {
      open = 0;
      mutexADC.unlock();
    }

    bool isLocked()
    {
      return open;
    }

  private:
    bool open;
    std::mutex mutexADC;
};



class ADC
{
  public:
    ADC(std::vector<AdcInputChannel>& channels) : adcChannels(channels) {}

    void requestADC(int c)
    {
      if (lock.isLocked()) 
      {
        std::string output = "ADC is locked, thread " + std::to_string(c) + " is about to suspend...\n";
        cout << output;

        std::unique_lock <std::mutex> adcLock (mtx);

        while(lock.isLocked())
          cond_var.wait(adcLock);

        requestADC(c);
      } 
      else 
      {
        lock.lock();
        std::string output = "ADC locked by thread " + std::to_string(c) + "\n";
        cout << output;
      }
    }

    double sampleADC(int c)
    {
      return adcChannels[c].getCurrentSample();
    }

    void releaseADC(int c)
    {
      std::string output = "ADC unlocked by thread " + std::to_string(c) + "\n";
      cout << output;
      lock.unlock();

      std::unique_lock <std::mutex> adcLock (mtx);
      cond_var.notify_all();
    }

  private:
    Lock lock;
    std::vector<AdcInputChannel>& adcChannels; //vector reference
};



//run function –executed by each thread:
void run(ADC& adcChannels, int id)
{
  //create thread map
  std::unique_lock<std::mutex> map_locker(mu);
  //insert thread ID and id into the map:
  threadIDs.insert(std::make_pair(std::this_thread::get_id(), id));
  map_locker.unlock();

  // double sampleBlock[DATA_BLOCK_SIZE] = {0.0}; //only for A2

  for (int i=0; i<50; i++) 
  {
    int threadID = search();

    adcChannels.requestADC(threadID);
    std::string output = "    sample value from thread " + std::to_string(threadID) + " = " + std::to_string(adcChannels.sampleADC(threadID)) + "\n";
    cout << output;
    adcChannels.releaseADC(threadID);


    int n = dis(gen);

    std::this_thread::sleep_for(std::chrono::milliseconds(n));
  }
}


int search()
{
  std::map <std::thread::id, int>::iterator it = threadIDs.find(std::this_thread::get_id());
  if(it == threadIDs.end())
    return -1;
  else
    return it->second;
}


int main()
{
  //initialise the ADC channels:
  std::vector<AdcInputChannel> adcChannels = std::vector<AdcInputChannel>{{(AdcInputChannel(0)), (AdcInputChannel(1)), (AdcInputChannel(2)), (AdcInputChannel(3)), (AdcInputChannel(4)), (AdcInputChannel(5))}};

  ADC adcObj(adcChannels);

  //instantiate and start the threads:

  std::thread the_threads[MAX_NUM_OF_THREADS]; //array of threads


  for (int i = 0; i < MAX_NUM_OF_THREADS; i++)
  {
    the_threads[i] = std::thread(run, std::ref(adcObj), i);
  }

  for (int k = 0; k < MAX_NUM_OF_THREADS; k++)
  {
    the_threads[k].join();
  }

  //wait for the threads to finish:
  cout << "All threads terminated" << endl;
  return 0;
} 

