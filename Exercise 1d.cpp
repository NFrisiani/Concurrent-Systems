#include <iostream>
#include <thread>
using namespace std;

std::mutex m;

class TClass
{
	public:
		TClass(int x): y(x){}
		void run()
		{
			m.lock();
			for(int i = 0; i < 5; i++)
			{
				cout << "Hello, I am thread " << std::this_thread::get_id() << ",  y = " << y << endl; 
				y++;
			}
			m.unlock();
		}
	private:
		int y;
};


int main()
{
	std::thread t[6];

	TClass tc(0);

	for(int i =0; i<6; i++)
	{
		t[i] = std::thread(&TClass::run, std::ref(tc));
	}

	
	for(int i =0; i<6; i++)
	{
		t[i].join();
	}

	std::cout << "All threads Terminated" << endl;

}