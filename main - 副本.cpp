// block_io_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

using namespace std::chrono;

#define WINDOW_LENGTH   90
#define CHECK_TIME  2000

struct mren_block_io_time_window {
    int first_gear_start_index;
    int second_gear_start_index;
    int third_gear_start_index;

    int first_gear_block_io_num;
    int second_gear_block_io_num;
    int third_gear_block_io_num;

    int cursors;

    unsigned long window[WINDOW_LENGTH];

    mren_block_io_time_window() : first_gear_start_index(0), second_gear_start_index(0), third_gear_start_index(0),
        first_gear_block_io_num(0), second_gear_block_io_num(0), third_gear_block_io_num(0), cursors(-1)
    {

    }

};

static mren_block_io_time_window g_window;

void alarm()
{

}

void CheckIO(long long time)
{
    // firt gear
    for (auto i = 0; i < time / CHECK_TIME + 1; ++i) {

    }
    g_window.window[++g_window.cursors % WINDOW_LENGTH] = time;
    if (time >= 1000) {
        ++g_window.first_gear_block_io_num;
    }

    if (g_window.first_gear_block_io_num >= 4) {
        alarm();
    }

    int interval = g_window.cursors < g_window.first_gear_start_index ? g_window.cursors + WINDOW_LENGTH - g_window.first_gear_start_index + 1 :
        g_window.cursors - g_window.first_gear_start_index + 1;
    
    
    if (interval >= 5) {
        for (auto i = 0; i < interval - 5 + 1; ++i) {
            if (g_window.window[(g_window.first_gear_start_index + i) % WINDOW_LENGTH] >= 1000) {
                --g_window.first_gear_block_io_num;
            }
            g_window.first_gear_start_index = (g_window.first_gear_start_index + 1) % WINDOW_LENGTH;
        }
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

    CheckIO(end - begin);
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
