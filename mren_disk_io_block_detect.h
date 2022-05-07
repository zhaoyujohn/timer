#pragma once
#include <deque>
#include <string>
struct mren_disk_statistic {
    unsigned long rd_ios;       // 读完成次数
    unsigned long wr_ios;      // 写完成次数
    unsigned long rd_ticks;    // 读操作花费毫秒数
    unsigned long wr_ticks;     // 写操作花费的毫秒数
    unsigned long tot_ticks;     // IO操作花费的毫秒数
    unsigned long rq_ticks;      // IO操作花费的加权毫秒数
    unsigned long in_flight;     // 正在处理的IO请求数

    mren_disk_statistic() : rd_ios(0), wr_ios(0), rd_ticks(0), wr_ticks(0), tot_ticks(0), rq_ticks(0), in_flight(0)
    {}
};

template<typename T >
class mren_fault_detection_sliding_windows {
public:
    mren_fault_detection_sliding_windows(int check_thresold, int fault_thresold, std::function<bool(T&)>&& func) 
        : check_thresold_(check_thresold), fault_thresold_(fault_thresold), check_func_(func)
    {}
    ~mren_fault_detection_sliding_windows()
    {}
public:
    bool check_if_reach_fault_thresold(T& value)
    {
        if (!check_func_(value)) {
            ++fault_num_;
        }

        sliding_window_.push(value);
        while (sliding_window_.size() > check_thresold_) {
            if (!check_func_(sliding_window_.front())) {
                --fault_num_;
            }
            sliding_window_.pop();
        }

        if (fault_num_ >= fault_thresold_) {
            return true;
        }
        return false;
    }
private:
    std::queue<T> sliding_window_;
    int check_thresold_;
    int fault_thresold_;
    int fault_num_ = 0;
    std::function<bool(T&)> check_func_;
};

class mren_disk_io_block_detect
{
public:

private:

    std::string diskName_;

    mren_disk_statistic current_disk_statistic_;
    mren_disk_statistic previos_disk_statistic_;

    unsigned long svctm_;
    int block_io_num_;

    std::deque<bool> fail_io_sliding_window_;
    std::deque<unsigned> slow_io_sliding_window_;
};

