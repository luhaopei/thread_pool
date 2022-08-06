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
    std::deque<function_wrapper> work_queue;  // ��ʹ�� function_wrapper ���� std::        function
    
    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> // ��
        submit(FunctionType f)
    {
        typedef typename std::result_of<FunctionType()>::type result_type;// ��

        std::packaged_task<result_type()> task(std::move(f)); // ��
        std::future<result_type> res(task.get_future());  // ��
        work_queue.push_back(std::move(task)); // ��
        return res; // ��
    }
};