// block_io_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <queue>
#include <thread>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

using namespace std::chrono;

#define WINDOW_LENGTH   10
#define CHECK_TIME  2000
#define FAULT_NUM   5

struct mren_io_time {
    unsigned long absolutleTime;
    unsigned int ioTime;

    mren_io_time() : absolutleTime(0), ioTime(0)
    {}
};

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

struct mren_iostat {
    unsigned long svctm;
    bool is_block_io;

    mren_iostat() : svctm(0), is_block_io(0)
    {}
};

struct mren_block_io_time_window {
    int failNum;
    int checkNum;
    std::queue<bool> window;

    mren_block_io_time_window() : failNum(0), checkNum(10)
    {
    }

};

static mren_block_io_time_window g_window;
static mren_disk_statistic g_pstat;
static mren_disk_statistic g_cstat;

void alarm()
{

}

void CheckIO(bool isSucceed)
{
    if (!isSucceed) {
        ++g_window.failNum;
    }

    g_window.window.push(isSucceed);
    while (g_window.window.size() >= g_window.checkNum) {
        if (!g_window.window.front()) {
            --g_window.failNum;
        }
        g_window.window.pop();
    }

    if (g_window.failNum >= FAULT_NUM) {
        alarm();
    }
}

void iotest()
{
    static unsigned long cursors = 0;
    /*static int partitionNum = 40960;*/
    static int partitionNum = 10;
    static std::string buf = "123";
    char temp[64] = { 0 };

    int fd = open("/home/file", O_RDWR);
    auto curr = (cursors++) % partitionNum * buf.length();
    auto ret = lseek(fd, curr, SEEK_SET);
    ret = write(fd, buf.data(), buf.length());
    ret = lseek(fd, curr, SEEK_SET);
    ret = read(fd, temp, buf.length());
    close(fd);
    std::cout << temp << std::endl;

    CheckIO(false);
}

void worker()
{
    auto sleepTime = std::chrono::milliseconds(CHECK_TIME);
    while (true) {
        auto nextTimeout = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch() + sleepTime;
        iotest();
        auto now = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch();
        auto waitTime = (now >= nextTimeout) ? sleepTime - (now - nextTimeout) % sleepTime : (nextTimeout - now);
        std::this_thread::sleep_for(waitTime);
        std::cout << waitTime.count() << std::endl;
    }
}

bool getDiskStat(mren_disk_statistic& stat)
{
    std::ifstream input("/proc/diskstats");
    if (!input) {
        return false;
    }

    char buffer[256] = { 0 };

    while (input.getline(buffer, sizeof(buffer))) {
        std::cout << buffer << std::endl;
    }

}

bool check_block_io()
{
    return true;
}

bool check_slow_io()
{
    return true;
}

void iostat()
{
    getDiskStat(g_cstat);

    if (check_block_io()) {
        return;
    }

    if (check_slow_io()) {
        return;
    }
}

void func()
{
    getDiskStat(g_pstat);
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        iostat();
    }
}

int main()
{
    std::thread thrd(func);
    thrd.join();
    worker();
    std::cout << "Hello World!\n";
}
