#ifndef THREAD_POOL_H
#define  THREAD_POOL_H

#include <thread>
#include <utility>
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <queue>
#include <exception>
#include <future>

namespace zy{
	class threadpool
	{
	public:
		threadpool(size_t n):mExit(false),mThreadNum(n){
			init();
		}
		~threadpool(){
			kill_all();
		}

		void resize(size_t n)
		{
			mThreadNum = n;
			if(vWorker.size()>0){
				kill_all();
				vWorker.clear();
			}
			mExit.store(false,std::memory_order_release);
			init();
		}

		template<typename F>
		auto submitTask(F&& f)->std::future<typename std::result_of<F()>::type>
		{			
			if (mExit.load(std::memory_order_acquire) || !mThreadNum)
				throw std::runtime_error("cannot submitTask");
				//return std::future<typename std::result_of<F()>::type>();
				
			typedef typename std::result_of<F()>::type return_type;
			auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f)));
			std::future<return_type> res = task->get_future();
			{
				std::unique_lock<std::mutex> sLock(mQueueLock);
				mTasks.emplace([task](){(*task)();});
			}
			mCondTask.notify_one();
			return res;
		}

	private:
		std::vector<std::thread> vWorker;          
		std::queue<std::function<void()>> mTasks;		
		std::mutex mQueueLock;
		std::condition_variable_any mCondTask;
		std::atomic<bool> mExit;
		size_t mThreadNum;
	private:
		threadpool(){};
		threadpool(const threadpool &p) {} 
		threadpool(threadpool&& p){}
		threadpool& operator=(const threadpool& p){}
		void init()
		{
			for(size_t i=0;i<mThreadNum;++i){
				vWorker.emplace_back([this](){
					while(1){
						std::function<void()> fn;
						{
							std::unique_lock<std::mutex> sLock(this->mQueueLock);
							threadpool* p = this;	
							mCondTask.wait(sLock,[p](){return p->mExit.load(std::memory_order_acquire) || !p->mTasks.empty();});
							if(mExit.load(std::memory_order_acquire)) break;
							if(p->mTasks.empty()) continue;
							fn = std::move(mTasks.front());
							mTasks.pop();
						}
						fn();
					}
				});
			}
		}
		void kill_all()
		{
			mExit.store(true,std::memory_order_release);
			{
				std::unique_lock<std::mutex> sLock(mQueueLock);
				mTasks = std::queue<std::function<void()>>();
			}
			mCondTask.notify_all();
			for(auto& item : vWorker){
				item.join();
			}
		}
	};
}

#endif