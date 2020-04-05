#include "timerwheel.h"
#include <iostream>
using namespace std;
using namespace std::chrono;

static int wheelSize = 1024;

timerwheel::timerwheel()
	: timerID(0),wheel_(wheelSize), exitFlag_(false)
{
	workerThread_ = std::make_unique<std::thread>(&timerwheel::run, this);
}

timerwheel::~timerwheel()
{

}

unsigned long timerwheel::addTimer(unsigned long time)
{

	return 0;
}

bool timerwheel::deleteTimer(unsigned long id)
{
	return false;
}

void timerwheel::run()
{
	base_ = std::chrono::steady_clock::now();
	while (!exitFlag_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		auto current = std::chrono::steady_clock::now();
		auto inter = std::chrono::duration_cast<std::chrono::milliseconds>(current - base_).count();
		cout << inter << endl;
		auto iii = std::chrono::duration_cast<std::chrono::milliseconds>(current.time_since_epoch()).count();
		base_ = base_;
	}
}
