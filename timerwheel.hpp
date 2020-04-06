#pragma once

#include <utility>
#include <chrono>
#include <functional>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>

namespace zy {

	struct timer_id
	{
		long long expireTime_;
		long long id_;
		std::function<void()> func_;
		timer_id() : expireTime_(0), id_(-1) {}
		timer_id(long long time, long long id, std::function<void()>&& func)
			: expireTime_(time), id_(id), func_(func) {}
	};

	class timerwheel
	{
	public:
		timerwheel() : timerID_(1), wheel_(wheelSize), base_(0), exitFlag_(false)
		{
			workerThread_ = std::make_unique<std::thread>(&timerwheel::run, this);
		}

		~timerwheel()
		{
			exitFlag_ = true;
			workerThread_->join();
		}

		template<typename F, typename... Args>
		std::shared_ptr<timer_id> addTimer(unsigned int delay, F&& f, Args&&... args)
		{
			auto expire = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() + delay;
			auto index = expire & wheelMask;
			auto key = expire >> wheelBitNum;
			auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
			auto id = timerID_.fetch_add(1, std::memory_order_release);
			auto res = std::make_shared<timer>(expire, id, [task]() {task(); });
			{
				std::lock_guard<std::mutex> lock(mutex_);
				wheel_[index][key].push_back(res);
			}

			return res;
		}

		bool deleteTimer(std::shared_ptr<timer_id>& id)
		{
			{
				std::lock_guard<std::mutex> lock(mutex_);
				auto index = id->expireTime_ & wheelMask;
				auto key = id->expireTime_ >> wheelBitNum;
				auto iter = wheel_[index].find(key);
				if (iter == wheel_[index].end())
					return true;
				for (auto iterTimer = iter->second.begin(); iterTimer != iter->second.end(); ++iter) {
					if ((*iterTimer)->g == )
				}
				for (auto iter = timers.begin(); iter != timers.end(); ) {
					if (iter->first > key)
						break;
					executeTimer.push_back(std::move(iter->second));
					iter = timers.erase(iter);
				}
			}
			return true;
		}

	private:
		void run()
		{
			base_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
			std::vector<std::list<std::shared_ptr<timer_id>>> executeTimer;
			while (!exitFlag_) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));

				auto current = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
				while (base_ <= current) {
					++base_;
					int index = base_ & wheelMask;
					auto key = base_ >> wheelBitNum;
					auto& timers = wheel_[index];

					{
						std::lock_guard<std::mutex> lock(mutex_);
						for (auto iter = timers.begin(); iter != timers.end(); ) {
							if (iter->first > key)
								break;
							executeTimer.push_back(std::move(iter->second));
							iter = timers.erase(iter);
						}
					}

					for (auto& item : executeTimer) {
						for (auto iter = item.begin(); iter != item.end(); ++iter)
							(*iter)->func_();
					}
					executeTimer.clear();
				}
			}
		}

	private:
		static const int wheelSize = 1024;
		static const int wheelBitNum = 10;
		static constexpr int wheelMask = wheelSize - 1;

		typedef  std::map<unsigned long, std::list<std::shared_ptr<timer_id>>> timer_tree;

		std::atomic<long long> timerID_;
		long long base_;
		std::vector<timer_tree> wheel_;
		std::unique_ptr<std::thread> workerThread_;
		volatile bool exitFlag_;
		std::mutex mutex_;
	};
}

 
