/*

Author: Nicolo Frisiani

Subject: Concurrent Systems

Assignment: Laboratory 2, part A2

The code simulates 6 threads accessing a single ADC and 
transmitting the values sampled from the ADC to a receiver 
via 3 different available links.

*/


//File inclusions
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <random>
#include <string>
#include <condition_variable>
using namespace std;

//Function declaration
int search();

//Global Variables
int const MAX_NUM_OF_CHAN = 6;
int const MAX_NUM_OF_THREADS = 6;
int const DATA_BLOCK_SIZE = 20;
int const NUM_OF_LINKS = 3;

//mutex used for sampling ADC
std::mutex mutexADC;

//used to suspend threads when requesting a busy ADC
std::mutex mtx;
std::condition_variable cond_var;

//used to suspend threads when requesting link and all links are busy
std::mutex mt;
std::condition_variable cond;

//used to create the map of threads
std::mutex mu;
std::map<std::thread::id, int> threadIDs;

//used to generate random delays between 0.1 and 0.5 seconds.
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(100,500);


//Receiver class: received data blocks from the threads and stores 
//them in a 2D array that will be printed at the end of the program
class Receiver 
{     
  public:       
  //constructor:       
    Receiver () //syntax -no initialiser list, no ":" 
    {            
      //initialise dataBlocks:  
      for(int i=0; i<MAX_NUM_OF_THREADS; i++)
      {
        for(int j=0; j<DATA_BLOCK_SIZE; j++)
        {
          dataBlocks[i][j] = 0;
        }
      }                
    }        

    //Receives a block of doubles such that the data   
    //is stored in index id of dataBlocks[][]       
    void receiveDataBlock(int id, double data[]) 
    {     
      for(int i = 0; i < DATA_BLOCK_SIZE; i++)
      {
        dataBlocks[id][i] = data[i];
      }      
    }          

    // print out all items       
    void printBlocks()
    {   

      cout << "The receiver data array is as follows: " << endl;
      cout << endl;

      for(int i=0; i<MAX_NUM_OF_THREADS; i++)    //This loops on the rows.
      {
        cout << "Thread " << i << ": \t";
        for(int j=0; j<DATA_BLOCK_SIZE; j++) //This loops on the columns
        {
          cout << dataBlocks[i][j]  << "\t";
        }
        cout << endl;
      } 
    }

  private:       
    double dataBlocks[MAX_NUM_OF_THREADS][DATA_BLOCK_SIZE];
}; //end class Receiver  


//Class for individual links
class Link 
{
  public:       
    Link (Receiver& r, int linkNum) //Constructor        
      : inUse(false), myReceiver(r), linkId(linkNum) {}        

    //check if the link is currently in use       
    bool isInUse() 
    {         
      return inUse;      
    }        

    //set the link status to busy       
    void setInUse() 
    {         
      inUse = true;       
    }        

    //set the link status to idle       
    void setIdle() 
    {         
      inUse = false;       
    }        

    //write data[] to the receiver       
    void writeToDataLink(int id, double data[]) 
    {        
      myReceiver.receiveDataBlock(id, data);
    }        

    //returns the link Id       
    int getLinkId() 
    {         
      return linkId;       
    }      

  private:       
  bool inUse;       
  Receiver& myReceiver;  //Receiver reference       
  int linkId; 
}; 



//Call for the controller regulating access to individual links
class LinkAccessController 
{     
  public:             
    LinkAccessController(Receiver& r)   //Constructor        
      :  myReceiver(r), numOfAvailableLinks(NUM_OF_LINKS)        
      {           
        for (int i = 0; i < NUM_OF_LINKS; i++) 
        {               
          commsLinks.push_back(Link(myReceiver, i));   //initialises links         
        }  
      }  

    //Request a comm's link: returns an available Link.       
    //If none are available, the calling thread is suspended.       
    Link requestLink() 
    {         
      int threadID = search();  

      int linkNum;   

      //check if there are any links available
      if(numOfAvailableLinks == 0)
      {
        std::string outputSus = "All Links are taken, thread " + std::to_string(threadID) + " is about to suspend...\n";
        cout << outputSus;

        std::unique_lock <std::mutex> linkLock (mt);

        //Suspend threads when no link is available
        while(numOfAvailableLinks == 0)
          cond.wait(linkLock);

        //When threads stop being suspended call requestLink again
        return requestLink();
      }
      else //check which link is available (starting from link 0)
      {
        for(int i = 0; i < NUM_OF_LINKS; i++)
        {
          if(!commsLinks[i].isInUse()) //If link i is not busy 
          {
            linkNum = i; 

            commsLinks[linkNum].setInUse(); //set link i to busy

            numOfAvailableLinks--; //link i is no longer available

            std::string output = "Thread " + std::to_string(threadID) + " accessed link " + std::to_string(linkNum) + "\n";
            cout << output;

            break; //break out of the for loop because a link has already been accessed

          }
        }
      }
      
      return commsLinks[linkNum]; //return the link to be accessed
    }        

    //Release a comms link:       
    void releaseLink(Link& releasedLink) 
    {    
      int id = releasedLink.getLinkId(); //get ID of the link 
      int threadID = search();

      std::string output = "Thread " + std::to_string(threadID) + " released link " + std::to_string(id) + "\n";
      cout << output;

      commsLinks[id].setIdle(); //release the link
      numOfAvailableLinks++; //link id is now available again

      //communicate to the waiting threads to stop waiting
      std::unique_lock <std::mutex> linkLock (mt);
      cond.notify_all();
    }

  private:       
    Receiver& myReceiver; //Receiver reference       
    int numOfAvailableLinks;       
    std::vector<Link> commsLinks;
}; 


//Class simulating the ADC input
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


//Class handling the locking and unlocking of the ADC
class Lock
{
  public:
    Lock() : open(false) {}

    void lock() //returns a flag to indicate when a thread should be blocked
    {
      open = 1;
      mutexADC.lock(); //locks the ADC
    }

    void unlock()
    {
      open = 0;
      mutexADC.unlock(); //unlocks the ADC
    }

    bool isLocked()
    {
      return open;
    }

  private:
    bool open;
    std::mutex mutexADC;
};


//Class for the simulated ADC
class ADC
{
  public:
    ADC(std::vector<AdcInputChannel>& channels) : adcChannels(channels) {}

    void requestADC(int c)
    {
      if (lock.isLocked()) //if the ADC is locked
      {
        std::string output = "ADC is locked, thread " + std::to_string(c) + " is about to suspend...\n";
        cout << output;

        std::unique_lock <std::mutex> adcLock (mtx);

        //Suspend threads until ADC gets unlocked
        while(lock.isLocked())
          cond_var.wait(adcLock);

        //when threads are 'released' call the requestADC() method again
        requestADC(c);
      } 
      else //if the ADC is unlocked
      {
        lock.lock(); //lock ADC
        std::string output = "ADC locked by thread " + std::to_string(c) + "\n";
        cout << output;
      }
    }

    double sampleADC(int c) //Sample adc channel (channel # is the same as thread ID)
    {
      return adcChannels[c].getCurrentSample();
    }

    void releaseADC(int c)
    {
      std::string output = "ADC unlocked by thread " + std::to_string(c) + "\n";
      cout << output;
      lock.unlock(); //unlock ADC

      //notify all waiting threads that ADC has been unlocked
      std::unique_lock <std::mutex> adcLock (mtx);
      cond_var.notify_all();
    }

  private:
    Lock lock;
    std::vector<AdcInputChannel>& adcChannels; //vector reference
};



//run function –executed by each thread:
void run(ADC& adcChannels, int id, LinkAccessController& lac)
{
  //create thread map
  std::unique_lock<std::mutex> map_locker(mu);
  //insert thread ID and id into the map:
  threadIDs.insert(std::make_pair(std::this_thread::get_id(), id));
  map_locker.unlock();

  double sampleBlock[DATA_BLOCK_SIZE] = {0.0}; 

  for (int i=0; i<DATA_BLOCK_SIZE; i++) //20 iterations
  {
    int threadID = search(); //take ID of current thread

    //request ADC 
    adcChannels.requestADC(threadID);

    //sample ADC and store result in appropriate sample # of sampleBlock
    sampleBlock[i] = adcChannels.sampleADC(threadID);

    std::string output = "sample value from thread " + std::to_string(threadID) + " = " + std::to_string(sampleBlock[i]) + "\n";
    cout << output;

    //release ADC 
    adcChannels.releaseADC(threadID);

    //delay by a random amount between 0.1 and 0.5 seconds
    int n = dis(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(n));

    //END OF PART A1 LOOP

    //BEGIN OF PART A2 LOOP

    std::string output1 = "Thread " + std::to_string(threadID) + " is trying to access a link... \n";
    cout << output1;
    
    //request a Link
    Link link = lac.requestLink(); 

    //delay by a random amount between 0.1 and 0.5 seconds
    int k = dis(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(k));

    std::string output2 = "Thread " + std::to_string(threadID) + " is transmitting sampled data to the receiver \n";
    cout << output2;

    //Write data in sampleBlock to the receiver (via the link) 
    link.writeToDataLink(threadID, sampleBlock);

    std::string output3 = "Thread " + std::to_string(threadID) + " is releasing the link... \n";
    cout << output3;

    //release Link
    lac.releaseLink(link);

  }
}

//method to find ID of a thread
int search()
{
  std::map <std::thread::id, int>::iterator it = threadIDs.find(std::this_thread::get_id());
  
  //taking it from the map, returns the ID of the thread calling the method
  if(it == threadIDs.end())
    return -1;
  else
    return it->second;
}


int main()
{
  //initialise the ADC channels:
  std::vector<AdcInputChannel> adcChannels;

  for(int i = 0; i < MAX_NUM_OF_CHAN; i++)
  {
    adcChannels.push_back(i);
  }

  ADC adcObj(adcChannels);

  Receiver rcv;

  LinkAccessController lac(rcv);


  //instantiate and start the threads:
  std::thread the_threads[MAX_NUM_OF_THREADS]; //array of threads

  //launch threads
  for (int i = 0; i < MAX_NUM_OF_THREADS; i++)
  {
    the_threads[i] = std::thread(run, std::ref(adcObj), i, std::ref(lac));
  }

  //join threads
  for (int k = 0; k < MAX_NUM_OF_THREADS; k++)
  {
    the_threads[k].join();
  }

  //wait for the threads to finish:
  cout << "All threads terminated" << endl;

  //print receiver array
  rcv.printBlocks();

  return 0;
} 