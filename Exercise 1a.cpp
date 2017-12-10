#include <iostream>
#include <thread>
#include <mutex> 
using namespace std;


void run()
{
	for(int i = 0; i < 5; i++)
		cout << "Hello, I am thread " << std::this_thread::get_id() << endl;
}


int main()
{
	std::thread t[6];

	for(int i =0; i<6; i++)
	{
		t[i] = std::thread(run);
	}
	
	for(int i =0; i<6; i++)
	{
		t[i].join();
	}

	std::cout << "All threads Terminated" << endl;

}