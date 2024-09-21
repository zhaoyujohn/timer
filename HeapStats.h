/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: heap profile
 */
#ifndef HEAP_STATS_H_
#define HEAP_STATS_H_

#include <vector>
#include <memory>
#include <unordered_map>
#include <utility>

namespace HeapProfile {
    struct HeapStatsKey {
        std::vector<void*> stack_;

        HeapStatsKey() {}
        HeapStatsKey(const std::vector<void*>& stack) : stack_(stack) {}
        HeapStatsKey(const std::vector<void*>&& stack) : stack_(stack) {}
        ~HeapStatsKey() {}

        HeapStatsKey(const HeapStatsKey& lhs)
        {
            stack_ = lhs.stack_;
        }

        HeapStatsKey(HeapStatsKey&& lhs) noexcept
        {
            stack_ = std::move(lhs.stack_);
        }

        HeapStatsKey& operator=(const HeapStatsKey& rhs)
        {
            stack_ = rhs.stack_;
            return *this;
        }

        HeapStatsKey& operator=(HeapStatsKey&& rhs) noexcept
        {
            stack_ = std::move(rhs.stack_);
            return *this;
        }

        bool operator==(const HeapStatsKey& lhs) const
        {
            return stack_ == lhs.stack_;
        }
    };

    struct HeapPtrKey {
        void* ptr_;
        size_t bytes_;
        bool ismmap_;

        HeapPtrKey() : ptr_(nullptr), bytes_(0), ismmap_(false) {}
        HeapPtrKey(void* ptr) : ptr_(ptr), bytes_(0), ismmap_(false) {}
        HeapPtrKey(void* ptr, size_t bytes, bool ismmap) : ptr_(ptr), bytes_(bytes), ismmap_(ismmap) {}
        ~HeapPtrKey() {}

        bool operator==(const HeapPtrKey& lhs) const
        {
            return ptr_ == lhs.ptr_;
        }
    };

    struct HeapStatsItem {
        long allocs_;
        long allocsize_;
        long frees_;
        std::weak_ptr<HeapStatsKey> key_;

        HeapStatsItem() : allocs_(0), allocsize_(0), frees_(0) {}

        HeapStatsItem(long allocs, long allocsize, long frees)
			: allocs_(allocs), allocsize_(allocsize), frees_(frees) {}

        HeapStatsItem(long allocs, long allocsize, long frees, std::weak_ptr<HeapStatsKey>& key)
            : allocs_(allocs), allocsize_(allocsize), frees_(frees), key_(key)
        {}

        HeapStatsItem(long allocs, long allocsize, long frees, std::shared_ptr<HeapStatsKey>& key)
            : allocs_(allocs), allocsize_(allocsize), frees_(frees), key_(key)
        {}

        ~HeapStatsItem() {}
    };

    struct HeapDumpInfo {
        long alloc_;
        long allocsize_;
        long frees_;
        std::shared_ptr<HeapStatsKey> stackInfo_;

        HeapDumpInfo() : alloc_(0), allocsize_(0), frees_(0) {}
        HeapDumpInfo(long allocs, long allocsize, long frees) : alloc_(allocs), allocsize_(allocsize), frees_(frees) {}
        HeapDumpInfo(long allocs, long allocsize, long frees, const std::shared_ptr<HeapStatsKey>& stackInfo)
            : alloc_(allocs), allocsize_(allocsize), frees_(frees), stackInfo_(stackInfo) {}
        ~HeapDumpInfo() {}
    };

    struct HeapStatsEqualKey {
        bool operator () (const std::shared_ptr<HeapStatsKey>& lhs, const std::shared_ptr<HeapStatsKey>& rhs) const
        {
            return lhs->stack_ == rhs->stack_;
        }
    };

    struct HeapStatsHashFunc {
        std::size_t operator()(const std::shared_ptr<HeapStatsKey>& key) const
        {
                size_t hash = 0;
                for (auto& item : key->stack_) {
                    hash += reinterpret_cast<size_t>(item);
                    hash += hash << 10; // 10是移位
                    hash ^= hash >> 6; // 6是移位
                }
                hash += hash << 3; // 3是移位
                hash ^= hash >> 11; // 11是移位
                return hash;
        }
    };
}

namespace std {
    template<>
    struct hash<HeapProfile::HeapPtrKey> {
        std::size_t operator()(const HeapProfile::HeapPtrKey& key) const
        {
            return std::hash<size_t> {}(reinterpret_cast<size_t>(key.ptr_));
        }
    };
}

#endif
