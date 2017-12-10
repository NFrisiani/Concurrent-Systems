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
using namespace std;

//Function declaration
int search();
//void run(ADC& theADC, int id);

//Global Variables
int const MAX_NUM_OF_CHAN = 6;
int const MAX_NUM_OF_THREADS = 6;
int const DATA_BLOCK_SIZE = 20;

std::mutex mu;

std::mutex threadID_mu;
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
		Lock() : open(true) {}

		bool lock() //returns a flag to indicate when a thread should be blocked
		{
			return 1;
		}

		void unlock()  
		{

		}

	private:
		bool open;
};



class ADC
{
	public: 
		ADC(std::vector<AdcInputChannel>& channels) : adcChannels(channels) {}

		void requestADC(int c)
		{
			cout << "ADC is locked by thread " << search() << endl;
		} 

		double sampleADC()
		{
			return adcChannels[search()].getCurrentSample();
		}

		void releaseADC()
		{
			cout << "ADC unlocked by thread " << search() << endl;
		}

	private:
		Lock theADCLock;
		int sampleChannel;
		std::vector<AdcInputChannel>& adcChannels; //vector reference
}; 



//run function –executed by each thread:
void run(ADC& theADC, int id) 
{
	//create thread map
	std::unique_lock<std::mutex> map_locker(threadID_mu);
	//insert thread ID and id into the map:
	threadIDs.insert(std::make_pair(std::this_thread::get_id(), id));
	map_locker.unlock();


	// double sampleBlock[DATA_BLOCK_SIZE] = {0.0}; //only for A2 

	for (int i=0; i<50; i++) 
	{

		theADC.requestADC(search());
	 
		cout << "sample value from thread" << search() << " = " << theADC.sampleADC() << endl;
	
		theADC.releaseADC();
	

		int n = dis(gen);

		std::this_thread::sleep_for(std::chrono::milliseconds(n));

		cout << "Delay by " << n << endl;
	}
}


int search()
{
	std::unique_lock<std::mutex> map_locker(threadID_mu);
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