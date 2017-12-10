#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

#define PSLEEP 100
#define CSLEEP 200

std::mutex m;

class Buffer
{
	public:
 		Buffer() : count(0) {}
		
		void put() 
		{
			m.lock();
			if(count < 10)
			{
				count++;
				cout << "Producer thread " << std::this_thread::get_id() << ", count = " << count << endl;
			}

			if(count == 10)
				cout << "Buffer is full, producer thread is about to suspend" << endl;
			m.unlock();
		}

		int get() 
		{
			m.lock();
			if(count > 0)
			{
				count--;
				cout << "Consumer thread " << std::this_thread::get_id() << ", count = " << count << endl;
			}

			if(count == 0)
				cout << "Buffer is empty, consumer thread is about to suspend" << endl;

			m.unlock();
			return 0;
		}

 	private:
 		int count;
};


class TClass
{
    public:
    	TClass(Buffer &b): buf(b) {}

        void prods()
        {
            for (int i=0; i<100; i++)
            {
            	buf.put();
                std::this_thread::sleep_for(std::chrono::milliseconds(PSLEEP));
            } 
        }
        void cons()
        {
            for (int i=0; i<100; i++)
            {
            	buf.get();
                std::this_thread::sleep_for(std::chrono::milliseconds(CSLEEP));
            }
        }

    private:
    	Buffer &buf;
};


int main()
{
	std::thread t[5];

	Buffer bu;
	TClass tc(bu);

	for(int i =0; i<3; i++)
		t[i] = std::thread(&TClass::prods, std::ref(tc));

	for(int k =3; k<5; k++)
		t[k] = std::thread(&TClass::cons, std::ref(tc));

	
	for(int p =0; p<5; p++)
		t[p].join();


	std::cout << "All threads Terminated" << endl;

}