#pragma once
#include <deque>
#include <string>
struct mren_disk_statistic {
    unsigned long rd_ios;       // ����ɴ���
    unsigned long wr_ios;      // д��ɴ���
    unsigned long rd_ticks;    // ���������Ѻ�����
    unsigned long wr_ticks;     // д�������ѵĺ�����
    unsigned long tot_ticks;     // IO�������ѵĺ�����
    unsigned long rq_ticks;      // IO�������ѵļ�Ȩ������
    unsigned long in_flight;     // ���ڴ����IO������

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

