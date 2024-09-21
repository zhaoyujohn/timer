/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: heap diagnose
 */

#include "HeapRecord.h"
#include <execinfo.h>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <functional>
#include <chrono>
#include <sys/prctl.h>
#include "HeapDump.h"

#define likely(x)       __builtin_expect((x), 1)
#define unlikely(x)     __builtin_expect((x), 0)

namespace HeapProfile {
    using namespace std::chrono;
    thread_local char HeapRecord::threadName[64] = {0};
    // 11 is max stack depth
    HeapRecord::HeapRecord() : maxDepth_(11), totalAllocSize_(0), dumpAllocInterval_(0), dumpTimeInterval_(-1)
    {
        threadpool_.reset(new HeapProfile::threadpool(1));
        Init();
    }

    HeapRecord::~HeapRecord()
    {
        HeapManage::DisableHeapRecord();
    }

    void HeapRecord::Init()
    {
    }

    void HeapRecord::Clear()
    {
        std::lock_guard<std::timed_mutex> lock(recordMutex_);
        totalAllocSize_ = 0;
        addressTable_.clear();
        heapTable_.clear();
        addressMmapTable_.clear();
    }

    void HeapRecord::GetStackTrace(int skip, std::vector<void*>& stack)
    {
        if (skip >= maxDepth_) {
            return;
        }

        std::vector<void*> buffer(maxDepth_);
        int nptrs = backtrace(buffer.data(), maxDepth_);
        if (nptrs <= skip) {
            return;
        }

        for (int i = 0; i < nptrs - skip; ++i) {            
            stack.push_back(buffer[skip + i]);
        }
    }

    size_t HeapRecord::GetMmapAllocSize(size_t bytes)
    {
        auto pagesize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        auto remainders = bytes % pagesize;
        if (remainders) {
            bytes = (bytes / pagesize + 1) * pagesize;
        }
        return bytes;
    }

    std::shared_ptr<HeapStatsKey> HeapRecord::GetHeapStatKey()
    {
        std::shared_ptr<HeapStatsKey> key = std::make_shared<HeapStatsKey>();
        GetStackTrace(3, key->stack_); // 3 is stack depth
        return key;
    }

    void HeapRecord::RecordAlloc(void* ptr, size_t bytes, bool isMmap)
    {
        if (unlikely(bytes == 0)) {
            return;
        }
        bool old = HeapManage::GetAndSetThrdRecordFlag(false);
        std::shared_ptr<HeapStatsKey> key = GetHeapStatKey();

        {
            std::lock_guard<std::timed_mutex> lock(recordMutex_);
            SaveAllocInfo(key, ptr, bytes, isMmap);
            DumpHeapProfile();
        }
        
        HeapManage::SetThrdRecordFlag(old);
    }

    void HeapRecord::RecordFree(void* ptr, size_t bytes, bool isMmap)
    {
        bool old = HeapManage::GetAndSetThrdRecordFlag(false);
        {
            std::lock_guard<std::timed_mutex> lock(recordMutex_);
            if (isMmap) {
                RecordMmapFree(ptr, bytes);
            }
            else {
                RecordMallocFree(ptr);
            }
        }
        HeapManage::SetThrdRecordFlag(old);
    }

    void HeapRecord::SaveAllocInfo(std::shared_ptr<HeapStatsKey>& key, void* ptr, size_t bytes, bool isMmap)
    {
        if (key->stack_.empty()) {
            return;
        }
        auto& addressTable = isMmap ? addressMmapTable_ : addressTable_;
        bytes = isMmap ? GetMmapAllocSize(bytes) : bytes;
        auto iter = heapTable_.find(key);
        if (iter == heapTable_.end()) {
            auto item = std::make_shared<HeapStatsItem>(1, bytes, 0);
            auto temp = heapTable_.insert({ key, item });
            item->key_ = temp.first->first;
            addressTable.insert({ HeapPtrKey(ptr, bytes, isMmap), item });
        } else {
            ++iter->second->allocs_;
            iter->second->allocsize_ += static_cast<long>(bytes);
            addressTable.insert({ HeapPtrKey(ptr, bytes, isMmap), iter->second });
        }
        totalAllocSize_ += static_cast<long>(bytes);
    }

    void HeapRecord::DumpHeapProfile()
    {
        bool isNeedDump = false;
        if (dumpAllocInterval_ > 0) {
            static long nextAllocSize = totalAllocSize_ + dumpAllocInterval_;
            if (totalAllocSize_ >= nextAllocSize) {
                nextAllocSize = totalAllocSize_ + dumpAllocInterval_;
                isNeedDump = true;
            }
        }
        
        if (dumpTimeInterval_ > 0) {
            auto now = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();
            static long nextDumpTime = now + dumpTimeInterval_;
            if (now >= nextDumpTime) {
                nextDumpTime = now + dumpTimeInterval_;
                isNeedDump = true;
            }
        }

        if (isNeedDump) {
            HeapProfile::HeapDump& ptr = HeapProfile::HeapDump::GetInstance();
            threadpool_->submitTask(std::bind(&HeapProfile::HeapDump::DumpHeapProfileToLog, &ptr, dumpPath_));
        }
    }

    void HeapRecord::RecordMallocFree(void* ptr)
    {
        auto iter = addressTable_.find(HeapPtrKey(ptr));
        if (iter == addressTable_.end()) {
            return;
        }

        ++iter->second->frees_;
        iter->second->allocsize_ -= iter->first.bytes_;
        if (iter->second->allocsize_ <= 0) {
            heapTable_.erase(iter->second->key_.lock());
        }
        totalAllocSize_ -= iter->first.bytes_;
        addressTable_.erase(iter);
    }

    void HeapRecord::RecordMmapFree(void* ptr, size_t bytes)
    {
        bytes = GetMmapAllocSize(bytes);
        auto iter = addressMmapTable_.find(HeapPtrKey(ptr));
        if (iter == addressMmapTable_.end()) {
            for (auto item = addressMmapTable_.begin(); item != addressMmapTable_.end(); ++item) {
                void* end = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(item->first.ptr_)
                    + reinterpret_cast<uintptr_t>(item->first.bytes_));
                if (ptr < item->first.ptr_ || ptr >= end) {
                    continue;
                }
                iter = item;
                break;
            }
            if (iter == addressMmapTable_.end()) {
                return;
            }
        }
        std::shared_ptr<HeapProfile::HeapStatsItem> heapStats = iter->second;
        auto oldEnd = reinterpret_cast<uintptr_t>(iter->first.ptr_) + reinterpret_cast<uintptr_t>(iter->first.bytes_);
        auto newEnd = reinterpret_cast<uintptr_t>(ptr) + reinterpret_cast<uintptr_t>(bytes);
        size_t firstBytes = reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(iter->first.ptr_);
        size_t secondBytes = oldEnd - newEnd;
        size_t freeBytes = ((newEnd > oldEnd) ? oldEnd : newEnd) - reinterpret_cast<size_t>(ptr);
        if (!firstBytes) {
            addressMmapTable_.erase(iter);
            iter = addressMmapTable_.end();
        } else {
            *const_cast<size_t*>(&iter->first.bytes_) = firstBytes;
        }

        if (newEnd < oldEnd) {
            addressMmapTable_.insert({HeapPtrKey(reinterpret_cast<void*>(newEnd), secondBytes, true), heapStats});
        }

        ++heapStats->frees_;
        heapStats->allocsize_ -= static_cast<long>(freeBytes);
        totalAllocSize_ -= static_cast<long>(freeBytes);
        if (heapStats->allocsize_ <= 0) {
            heapTable_.erase(heapStats->key_.lock());
        }
    }

    // capacity是dpumInfo最多可容纳的数量，默认值是1，表示dumpInfo没有内存规格限制
    bool HeapRecord::GetHeapProfileInfo(long& totalAlloc, std::vector<HeapDumpInfo>& dumpInfo, int capacity)
    {
        totalAlloc = 0;
        bool old = HeapManage::GetAndSetThrdRecordFlag(false);
        {
            std::unique_lock<std::timed_mutex> lock(recordMutex_, std::defer_lock);
            // 5 is sleep time
            if (!lock.try_lock_for(std::chrono::seconds(5))) {
                return false;
            }
            int i = 0;
            for (auto& item : heapTable_) {
                if (capacity > 0 && ++i >= capacity) {
                    break;
                }
                dumpInfo.emplace_back(item.second->allocs_, item.second->allocsize_, item.second->frees_, item.first);
            }
            totalAlloc = totalAllocSize_;
        }
        HeapManage::SetThrdRecordFlag(old);
        return true;
    }

    void HeapRecord::SetDumpAllocInterval(long value)
    {
        dumpAllocInterval_ = value;
    }

    long HeapRecord::GetDumpAllocInterval()
    {
        return dumpAllocInterval_;
    }

    void HeapRecord::SetDumpTimeInterval(int value)
    {
        dumpTimeInterval_ = value;
    }

    int HeapRecord::GetDumpTimeInterval()
    {
        return dumpTimeInterval_;
    }

    void HeapRecord::SetMaxStackDepth(int value)
    {
        if (value > 0) {
            maxDepth_ = value;
        }
    }

    int HeapRecord::GetMaxStackDepth()
    {
        return maxDepth_;
    }

    void HeapRecord::SetDumpPath(const std::string& path)
    {
        dumpPath_ = path;
    }

    const std::string& HeapRecord::GetDumpPath()
    {
        return dumpPath_;
    }
}
