/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: heap profile
 */
#ifndef HEAP_STATICS_H_
#define HEAP_STATICS_H_

#include <vector>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include "HeapStats.h"
#include "threadpool.hpp"
#include "HeapManage.h"

namespace HeapProfile {
    class HeapRecord {
    public:
        void RecordAlloc(void* ptr, size_t bytes, bool isMmap = false);

        void RecordFree(void* ptr, size_t bytes = 0, bool isMmap = false);

        bool GetHeapProfileInfo(long& totalAlloc, std::vector<HeapDumpInfo>& dumpInfo, int capacity = -1);
        void SetDumpAllocInterval(long value);
        long GetDumpAllocInterval();

        void SetDumpTimeInterval(int value);
        int GetDumpTimeInterval();

        void SetMaxStackDepth(int value);
        int GetMaxStackDepth();

        void SetDumpPath(const std::string& path);
        const std::string& GetDumpPath();

        static HeapRecord& GetInstance()
        {
            bool old = HeapManage::GetAndSetThrdRecordFlag(false);
            static HeapRecord instance;
            HeapManage::SetThrdRecordFlag(old);
            return instance;
        }

        static void StartHeapRecord()
        {
            HeapManage::EnableHeapRecord();
        }

        static void StopHeapRecord()
        {
            HeapManage::DisableHeapRecord();
            HeapRecord& item = HeapRecord::GetInstance();
            item.Clear();
        }

    private:
        HeapRecord();
        ~HeapRecord();
        void Init();
        void Clear();
        void GetStackTrace(int skip, std::vector<void*>& backtrace);
        std::shared_ptr<HeapStatsKey> GetHeapStatKey();
        size_t GetMmapAllocSize(size_t);
        void SaveAllocInfo(std::shared_ptr<HeapStatsKey>& stack, void* ptr, size_t bytes, bool isMmap);
        void DumpHeapProfile();
        void RecordMmapFree(void* ptr, size_t bytes);
        void RecordMallocFree(void* ptr);
    private:
        int maxDepth_;
        long totalAllocSize_;
        std::timed_mutex recordMutex_;
        long dumpAllocInterval_;
        int dumpTimeInterval_;
        std::string dumpPath_;
        thread_local static char threadName[64];
        std::unordered_map<std::shared_ptr<HeapStatsKey>, std::shared_ptr<HeapStatsItem>,
            HeapStatsHashFunc, HeapStatsEqualKey> heapTable_;
        std::unordered_map<HeapPtrKey, std::shared_ptr<HeapStatsItem>> addressTable_;
        std::unordered_map<HeapPtrKey, std::shared_ptr<HeapStatsItem>> addressMmapTable_;
        std::unique_ptr<HeapProfile::threadpool> threadpool_;
    };
}

#endif
