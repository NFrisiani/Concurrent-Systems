#include <iostream>
#include <thread>
using namespace std;

class TimedObject
{
	public:
		void doTimeAction()
		{
			cout << "Doing doTimeAction.." << endl;
			doNow = true;
		}
		void backGroundActivity()
		{
			for(int i = 0; i < 20; i++)
			{
				if(!doNow)
				{
					cout << "Doing backgroud activity.." << endl;
				} 
				else
				{
					cout << "Doing timed action" << endl;
					doNow = false;
				}
				std::this_thread::sleep_for (std::chrono::milliseconds(100));
			}
		}
	private:
		bool doNow = false;
};

class TimerT
{
	public:
		TimerT(TimedObject& t, int tD) : TO(t), timeInterval(tD), shouldRun(true){}
		void run()
		{
			int i = 0;
			while(shouldRun && i < 20)
			{
				cout << "invoking doTimeAction.." << endl;
				TO.doTimeAction();
				std::this_thread::sleep_for(std::chrono::milliseconds(timeInterval));
				i++;
			}
		}
	private:
		TimedObject& TO;
		int timeInterval;
		bool shouldRun;
};

int main()
{
	TimedObject aTo;
	TimerT timer(aTo, 100);
	std::thread t(&TimerT::run, &timer);
	aTo.backGroundActivity();
	t.join();

	std::cout << "End of program." << endl;

}