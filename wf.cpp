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


    // try to enqueue
    bool tryEnq(T &v, WfqEnqCtx<T> &ctx) {
        wfqindex head;
        T *currval, *newVal;
        std::atomic<T*> *nptrs;
        if (ctx.hasq_) {
            nptrs = ctx.nptr_;
            newVal = ctx.pendingNewVal_;
            currval = nptrs->load(std::memory_order_consume);
            for (int n = _WFQ_MAX_TRY_; n > 0; n--) {
                // if (!currval &&
                if ( !currval ) {
                    if (nptrs->compare_exchange_weak(currval, newVal,
                                                     std::memory_order_release,
                                                     std::memory_order_consume)) {
                        ctx.hasq_ = 0;
                        return true;
                    }
                } else {
                    atomic_thread_fence(std::memory_order_seq_cst);
                    currval = nptrs->load(std::memory_order_consume);
                }
            }
            return false;
        }
        newVal = new T(v);
        head = std::atomic_fetch_add_explicit(&head_, increase_one, std::memory_order_relaxed) % max_;
        nptrs = &nptr_[head];
        currval = nptrs->load(std::memory_order_consume);
        for (int n = _WFQ_MAX_TRY_; n > 0; n--) {
            if ( !currval ) {
                if (nptrs->compare_exchange_weak(currval, newVal,
                                                 std::memory_order_release,
                                                 std::memory_order_consume)) {
                    return true;
                }
            } else {
                atomic_thread_fence(std::memory_order_seq_cst);
                currval = nptrs->load(std::memory_order_consume);
            }
        }
        ctx.nptr_ = nptrs;
        ctx.pendingNewVal_ = newVal;
        ctx.hasq_ = 1;
        return false;
    }


    bool tryEnq(T *newVal, WfqEnqCtx<T> &ctx) {
        wfqindex head;
        T *currval;
        std::atomic<T*> *nptrs;
        if (ctx.hasq_) {
            nptrs = ctx.nptr_;
            currval = nptrs->load(std::memory_order_consume);
            for (int n = _WFQ_MAX_TRY_; n > 0; n--) {
                // if (!currval &&
                if ( !currval ) {
                    if (nptrs->compare_exchange_weak(currval, newVal,
                                                     std::memory_order_release,
                                                     std::memory_order_consume)) {
                        ctx.hasq_ = 0;
                        return true;
                    }
                } else {
                    atomic_thread_fence(std::memory_order_seq_cst);
                    currval = nptrs->load(std::memory_order_consume);
                }
            }
            return false;
        }
        head = std::atomic_fetch_add_explicit(&head_, increase_one, std::memory_order_relaxed) % max_;
        nptrs = &nptr_[head];
        currval = nptrs->load(std::memory_order_consume);
        for (int n = _WFQ_MAX_TRY_; n > 0; n--) {
            // if (!currval &&
            if ( !currval ) {
                if (nptrs->compare_exchange_weak(currval, newVal,
                                                 std::memory_order_release,
                                                 std::memory_order_consume)) {
                    return true;
                }
            } else {
                atomic_thread_fence(std::memory_order_seq_cst);
                currval = nptrs->load(std::memory_order_consume);
            }
        }
        ctx.nptr_ = nptrs;
        ctx.hasq_ = 1;
        return false;
    }

    bool tryDeq(T &v, WfqDeqCtx<T> &ctx) {
        wfqindex tail;
        std::atomic<T*> *nptrs;
        T *val;
        if (ctx.hasq_) {
            nptrs = ctx.nptr_;
            val = nptrs->load(std::memory_order_consume);
            for (int n = _WFQ_MAX_TRY_; n > 0; n--) {
                if ( val ) {
                    if (nptrs->compare_exchange_weak(val, nullptr,
                                                     std::memory_order_release,
                                                     std::memory_order_consume)) {
                        ctx.hasq_ = 0;
                        v = *val;
                        delete val;
                        return true;
                    }
                } else {
                    atomic_thread_fence(std::memory_order_seq_cst);
                    val = nptrs->load(std::memory_order_consume);
                }
            }
            return false;
        }

        tail = std::atomic_fetch_add_explicit(&tail_, increase_one, std::memory_order_relaxed) % max_;
        nptrs = &nptr_[tail];
        val = nptrs->load(std::memory_order_consume);
        for (int n = _WFQ_MAX_TRY_; n > 0; n--) {
            if ( val ) {
                if (nptrs->compare_exchange_weak(val, nullptr,
                                                 std::memory_order_release,
                                                 std::memory_order_consume)) {
                    v = *val;
                    delete val;
                    return true;
                }
            } else {
                atomic_thread_fence(std::memory_order_seq_cst);
                val = nptrs->load(std::memory_order_consume);
            }
        }

        ctx.nptr_ = nptrs;
        ctx.hasq_ = 1;
        return false;
    }

    T* tryDeq(WfqDeqCtx<T> &ctx) {
        wfqindex tail;
        std::atomic<T*> *nptrs;
        T *val;
        if (ctx.hasq_) {
            nptrs = ctx.nptr_;
            val = nptrs->load(std::memory_order_consume);
            for (int n = _WFQ_MAX_TRY_; n > 0; n--) {
                if ( val ) {
                    if (nptrs->compare_exchange_weak(val, nullptr,
                                                     std::memory_order_release,
                                                     std::memory_order_consume)) {
                        ctx.hasq_ = 0;
                        return val;
                    }
                } else {
                    atomic_thread_fence(std::memory_order_seq_cst);
                    val = nptrs->load(std::memory_order_consume);
                }
            }
            return nullptr;
        }

        tail = std::atomic_fetch_add_explicit(&tail_, increase_one, std::memory_order_relaxed) % max_;
        nptrs = &nptr_[tail];
        val = nptrs->load(std::memory_order_consume);
        for (int n = _WFQ_MAX_TRY_; n > 0; n--) {
            if ( val ) {
                if (nptrs->compare_exchange_weak(val, nullptr,
                                                 std::memory_order_release,
                                                 std::memory_order_consume)) {
                    return val;
                }
            } else {
                atomic_thread_fence(std::memory_order_seq_cst);
                val = nptrs->load(std::memory_order_consume);
            }
        }

        ctx.nptr_ = nptrs;
        ctx.hasq_ = 1;
        return nullptr;
    }

    inline void enq(T & v, WfqEnqCtx<T> &ctx) {
        while (!tryEnq(v, ctx))
            atomic_thread_fence(std::memory_order_seq_cst);
    }
    inline void enq(T * v, WfqEnqCtx<T> &ctx) {
        while (!tryEnq(v, ctx))
            atomic_thread_fence(std::memory_order_seq_cst);
    }

    inline void deq(T & v, WfqDeqCtx<T> &ctx) {
        while (!tryDeq(v, ctx))
            atomic_thread_fence(std::memory_order_seq_cst);
    }

    inline T* deq(WfqDeqCtx<T> &ctx) {
        T *v_;
        while ( !(v_ = tryDeq(ctx)) )
            atomic_thread_fence(std::memory_order_seq_cst);
        return v_;
    }

    inline size_t getSize() const {
        size_t _h = head_.load (std::memory_order_relaxed), _t = tail_.load(std::memory_order_relaxed);
        return _h > _t ? _h - _t : 0;
    }

    inline bool empty() const {
        return getSize() == 0;
    }

    ~Queue() noexcept {
        std::free(freebuf_);
    }
};


Queue<int> q(1024);
int test = 100;

void* producer(void* arg) {
    WfqEngCtx<int> ctx;
    for (int i = 0; i < test; ++i) {
        int* value = (int*)malloc(sizeof(int));
        *value = i;
        q.enq(value,ctx);
        std::cout << "Enqueued: " << *value << std::endl;
    }
    return NULL;
}

// 消费者线程函数
void* consumer(void* arg) {
    for (int i = 0; i < test; ++i) {
        int* value = ring_buffer_dequeue(&rb);
        if (value) {
            printf("Dequeued: %d\n", *value);
            free(value);
        } else {
            // 如果缓冲区空了，就稍等一会儿再试
            usleep(100);
        }
    }
    return NULL;
}

int main() {
    pthread_t prod, cons;


    pthread_create(&cons, NULL, consumer, NULL);
    pthread_create(&prod, NULL, producer, NULL);

    // 等待线程结束
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    return 0;
}
