#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <future>

#include "thread_safe_queue.h"
#include "function_wrapper.h"
#include "accumulate_block.h"
#include "work_stealing_queue.h"
#include "join_threads.h"

class thread_pool
{
    std::atomic_bool done;
    typedef function_wrapper task_type;
    thread_safe_queue<function_wrapper> pool_work_queue; 
    typedef std::queue<function_wrapper> local_queue_type;
    std::vector<std::unique_ptr<work_stealing_queue> > queues;
    std::vector<std::thread> threads;
    join_threads joiner;

    static thread_local work_stealing_queue* local_work_queue;
    static thread_local unsigned my_index_;

    void worker_thread(unsigned my_index)
    {
        my_index_ = my_index;
        local_work_queue = queues[my_index_].get();
        while (!done)
        {
            run_pending_task();
        }
    }

    bool pop_task_from_local_queue(task_type& task)
    {
        return local_work_queue && local_work_queue->try_pop(task);
    }

    bool pop_task_from_pool_queue(task_type& task)
    {
        return pool_work_queue.try_pop(task);
    }

    bool pop_task_from_other_thread_queue(task_type& task)
    {
        for (unsigned i = 0; i < queues.size(); ++i)
        {
            unsigned const index = (my_index_ + i + 1) % queues.size();
            if (queues[index]->try_steal(task))
            {
                return true;
            }
        }

        return false;
    }
public:
    thread_pool() :
        joiner(threads), done(false)
    {
        unsigned const thread_count = std::thread::hardware_concurrency();

        try
        {
            for (unsigned i = 0; i < thread_count; ++i)
            {
                queues.push_back(std::unique_ptr<work_stealing_queue>(
                    new work_stealing_queue));
                threads.push_back(
                    std::thread(&thread_pool::worker_thread, this, i));
            }
        }
        catch (...)
        {
            done = true;
            throw;
        }
    }

    ~thread_pool()
    {
        done = true;
    }


    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> // ② std::result_of<FunctionType>的实例即为函数 f()
        submit(FunctionType f)
    {
        typedef typename std::result_of<FunctionType()>::type result_type;// ③

        std::packaged_task<result_type()> task(f); // ④
        std::future<result_type> res(task.get_future());  // ⑤
        if (local_work_queue)
        {
            local_work_queue->push(std::move(task));
        }
        else
        {
            pool_work_queue.push(std::move(task));
        }
        return res; 
    }

    void run_pending_task()
    {
        task_type task;
        if (pop_task_from_local_queue(task) ||
            pop_task_from_pool_queue(task) ||
            pop_task_from_other_thread_queue(task))
        {
            task();
        }
        else
        {
            std::this_thread::yield();
        }
    }
};