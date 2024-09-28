#include <atomic>
#include <cstring>
#include<stdatomic.h>
#include<stdbool.h>
#include<stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include<stdlib.h>
#include<iostream>

#define _WFQ_MAX_TRY_ 128
// static unsigned long debug_cyc_count = 0;
// #define ADD_DEBUG_CYC_COUNT __sync_fetch_and_add(&debug_cyc_count, 1)
#define _WFQ_NULL_ 0
#define _WFQ_ALIGNED_SZ 128
#define _WFQ_CACHE_128_ALIGNED_ __attribute__((aligned(_WFQ_ALIGNED_SZ)))
#define _WFQ_MSVC_CACHE_128_ALIGNED_


typedef std::atomic_size_t atomic_wfqindex;
typedef size_t wfqindex;
static const size_t increase_one  =  1;


template<class eT>
struct _WFQ_MSVC_CACHE_128_ALIGNED_ _WFQ_CACHE_128_ALIGNED_ WfqEngCtx {
    unsigned hasq_: 1 _WFQ_MSVC_CACHE_128_ALIGNED_;
    eT *pendingNewVal_ _WFQ_CACHE_128_ALIGNED_;
    std::atomic<eT*> *nptr_ _WFQ_CACHE_128_ALIGNED_;

    WfqEnqCtx() {
        hasq_ = 0;
        pendingNewVal_ = nullptr;
        nptr_ = nullptr;
    }
};

template<class T>
class _WFQ_MSVC_CACHE_128_ALIGNED_ _WFQ_CACHE_128_ALIGNED_ Queue {
    private:
       atomic_wfqindex head_ _WFQ_CACHE_128_ALIGNED_;
       atomic_wfqindex tail_ _WFQ_CACHE_128_ALIGNED_;
       size_t max = _WFQ_CACHE_128_ALIGNED_;
       std::atomic<T*> *nptr_ _WFQ_CACHE_128_ALIGNED_;
       void *freebuf_;
   
    public:
       Queue(size_t capacity) {
        size_t total_sz = capacity * (sizeof(std::atomic<T*>) + _WFQ_ALIGNED_SZ);

        freebuf_ = (void*) std::malloc(total_sz + sizeof(void*));

        void *alloc_buf = (void*)(((uintptr_t) freebuf_) + sizeof(void*));

        nptr_ = reinterpret_cast<std::atomic<T*>> std::align(_WFQ_ALIGNED_SZ, capacity * sizeof(std::atomic<T*>), alloc_buf, total_sz);

        for (size_t i = 0; i < capacity; i++) {
            ntpr_[i] = ATOMIC_VAR_INIT(nullptr);
        }
        head_ = ATOMIC_VAR_INIT(0);
        tail_ = ATOMIC_VAR_INIT(0);
        max_  = capacity;
        atomic_thread_fence(std::memory_order_seq_cst);
       }

