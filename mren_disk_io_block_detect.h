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

