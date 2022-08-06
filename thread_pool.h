#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <future>

#include "thread_safe_queue.h"
#include "function_wrapper.h"


class thread_pool
{
    std::deque<function_wrapper> work_queue;  // ①使用 function_wrapper 而非 std::function
    
    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> // ② std::result_of<FunctionType>的实例即为函数 f()
        submit(FunctionType f)
    {
        typedef typename std::result_of<FunctionType()>::type result_type;// ③

        std::packaged_task<result_type()> task(std::move(f)); // ④
        std::future<result_type> res(task.get_future());  // ⑤
        work_queue.push_back(std::move(task)); // ⑥
        return res; // ⑦
    }

    /*
        我们需要把数据分成块，其体积是值得并发处理的最小尺寸，以便将线程池的可伸缩性利用到极致。当池中只有少量线程时，
        每个线程都会处理许多数据块，但如果硬件线程的数目有所增加，并行处理的块的数目也会随之变大。
    */

    template<typename Iterator, typename T>
    T parallel_accumulate(Iterator first, Iterator last, T init)
    {
        unsigned long const length = std::distance(first, last);

        if (!length)
            return init;

        unsigned long const block_size = 25;
        unsigned long const num_blocks = (length + block_size - 1) / block_size; // ①

        std::vector<std::future<T> > futures(num_blocks - 1);
        thread_pool pool;

        Iterator block_start = first;
        for (unsigned long i = 0; i < (num_threads - 1); ++i)
        {
            Iterator block_end = block_start;
            std::advance(block_end, block_size);
            futures[i] = pool.submit(accumulate_block<Iterator, T>()); // ②
            block_start = block_end;
        }
        T last_result = accumulate_block()(block_start, last);
        T result = init;
        for (unsigned long i = 0; i < (num_blocks - 1); ++i)
        {
            result += futures[i].get();
        }
        result += last_result;
        return result;
    }

};