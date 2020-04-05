#pragma once

#include <utility>
#include <chrono>
#include <functional>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <thread>

class timerwheel
{
public:
	timerwheel();
	~timerwheel();

	unsigned long addTimer(unsigned long time);
	bool deleteTimer(unsigned long id);

private:
	void run();

private:
	struct timer
	{
		unsigned long expireTime_;
		std::function<void()> func_;
	};
	typedef  std::map<unsigned long, std::list<timer>> timer_tree;

	unsigned long timerID;
	std::chrono::time_point<std::chrono::steady_clock> base_;
	std::vector<timer_tree> wheel_;
	std::unique_ptr<std::thread> workerThread_;
	volatile bool exitFlag_;
};
 
