#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

std::mutex m;

//if I pass the ID on from the main (like I did in 1a and 1b) for some reason it always passes the same id (look at commented code).
//if I try to retrieve the ID from the std::this_thread::get_id() if prints the memory location like it did in the other problems.
class TClass
{
	public:
		//TClass(int n): id(n){}
		void run()
		{
			//cout << "Hello, I am thread " << id << endl;   THIS DOESNT WORK. PRINTS LOCATION?
			m.lock();
			cout << "Hello, I am thread " << std::this_thread::get_id()<< endl << endl; 
			m.unlock();
		}
	//private:
		//int id;
};



int main()
{
	std::thread t[6];

	for(int i =0; i<6; i++)
	{
		TClass tc;
		//TClass tc(i+1);
		t[i] = std::thread(&TClass::run, &tc);
	}

	
	for(int i =0; i<6; i++)
	{
		t[i].join();
	}

	std::cout << "All threads Terminated" << endl;

}