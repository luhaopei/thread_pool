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
    std::deque<function_wrapper> work_queue;  // ①使用 function_wrapper 而非 std::        function
    
    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> // ②
        submit(FunctionType f)
    {
        typedef typename std::result_of<FunctionType()>::type result_type;// ③

        std::packaged_task<result_type()> task(std::move(f)); // ④
        std::future<result_type> res(task.get_future());  // ⑤
        work_queue.push_back(std::move(task)); // ⑥
        return res; // ⑦
    }
};