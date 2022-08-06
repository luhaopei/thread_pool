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
    std::deque<function_wrapper> work_queue;  // ��ʹ�� function_wrapper ���� std::function
    
    template <typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> // �� std::result_of<FunctionType>��ʵ����Ϊ���� f()
        submit(FunctionType f)
    {
        typedef typename std::result_of<FunctionType()>::type result_type;// ��

        std::packaged_task<result_type()> task(std::move(f)); // ��
        std::future<result_type> res(task.get_future());  // ��
        work_queue.push_back(std::move(task)); // ��
        return res; // ��
    }

    /*
        ������Ҫ�����ݷֳɿ飬�������ֵ�ò����������С�ߴ磬�Ա㽫�̳߳صĿ����������õ����¡�������ֻ�������߳�ʱ��
        ÿ���̶߳��ᴦ��������ݿ飬�����Ӳ���̵߳���Ŀ�������ӣ����д���Ŀ����ĿҲ����֮���
    */

    template<typename Iterator, typename T>
    T parallel_accumulate(Iterator first, Iterator last, T init)
    {
        unsigned long const length = std::distance(first, last);

        if (!length)
            return init;

        unsigned long const block_size = 25;
        unsigned long const num_blocks = (length + block_size - 1) / block_size; // ��

        std::vector<std::future<T> > futures(num_blocks - 1);
        thread_pool pool;

        Iterator block_start = first;
        for (unsigned long i = 0; i < (num_threads - 1); ++i)
        {
            Iterator block_end = block_start;
            std::advance(block_end, block_size);
            futures[i] = pool.submit(accumulate_block<Iterator, T>()); // ��
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