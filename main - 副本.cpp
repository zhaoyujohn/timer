// block_io_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>
#include <utility>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

using namespace std::chrono;

#define WINDOW_LENGTH   10
#define CHECK_TIME  2000

struct mren_io_time {
    unsigned long absolutleTime;
    unsigned int ioTime;

    mren_io_time() : absolutleTime(0), ioTime(0)
    {}
};

struct mren_block_io_time_window {
    int first_gear_start_index;
    int second_gear_start_index;
    int third_gear_start_index;

    int first_gear_block_io_num;
    int second_gear_block_io_num;
    int third_gear_block_io_num;

    int cursors;

    mren_io_time window[WINDOW_LENGTH];

    mren_block_io_time_window() : first_gear_start_index(0), second_gear_start_index(0), third_gear_start_index(0),
        first_gear_block_io_num(0), second_gear_block_io_num(0), third_gear_block_io_num(0), cursors(-1)
    {
    }

};

static mren_block_io_time_window g_window;

void alarm()
{

}

void CheckIO(long long start, long long end)
{
    // firt gear
    auto ioTime  = end - start;
    while (ioTime >= 0) {
        g_window.cursors = (g_window.cursors + 1) % WINDOW_LENGTH;
        g_window.window[g_window.cursors].absolutleTime = end - ioTime;
        g_window.window[g_window.cursors].ioTime = ioTime / CHECK_TIME ? CHECK_TIME : ioTime;
        
        int distance = (WINDOW_LENGTH + g_window.first_gear_start_index - g_window.cursors - 1) % WINDOW_LENGTH;
        if (distance == 0) {
            if (g_window.window[g_window.first_gear_start_index].ioTime >= 1000) {
                --g_window.first_gear_block_io_num;
            }
            g_window.first_gear_start_index = (g_window.first_gear_start_index + 1) % WINDOW_LENGTH;
        }

        if (g_window.window[g_window.cursors].ioTime >= 1000) {
            ++g_window.first_gear_block_io_num;
        }
        ioTime -= CHECK_TIME;
    }

    while (g_window.window[g_window.cursors].absolutleTime - g_window.window[g_window.first_gear_start_index].absolutleTime > 10000) {
        if (g_window.window[g_window.first_gear_start_index].ioTime >= 1000) {
            --g_window.first_gear_block_io_num;
        }
        g_window.first_gear_start_index = (g_window.first_gear_start_index + 1) % WINDOW_LENGTH;
    }

    if (g_window.first_gear_block_io_num >= 4) {
        alarm();
    }
}

void iotest()
{
    static unsigned long cursors = 0;
    static int partitionNum = 40960;
    static char buf[512] = { 0 };

    auto begin = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();
    int fd = open("/home/file", O_RDWR);
    auto ret = lseek(fd, (cursors++) % partitionNum, SEEK_SET);
    ret = write(fd, buf, sizeof(buf));
    ret = read(fd, buf, sizeof(buf));
    close(fd);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    auto end = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();

    CheckIO(begin, end);
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

int main()
{
    worker();
    std::cout << "Hello World!\n";
}
