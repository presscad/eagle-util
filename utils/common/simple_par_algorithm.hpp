#ifndef _SIMPLE_PAR_ALGORITHM_H
#define _SIMPLE_PAR_ALGORITHM_H

#include <memory>
#include <algorithm>
#include <thread>
#include <future>

namespace util {

template<class Iter, class Pr> inline
void ParSortImpl1(Iter begin, Iter end, Pr pred, int TASK_COUNT)
{
    auto count = std::distance(begin, end);
    if (count <= 1024 * 4) { // threshold
        std::sort(begin, end, pred);
        return;
    }

    if (TASK_COUNT > 2 && TASK_COUNT < 4) {
        TASK_COUNT = 2;
    }
    else if (TASK_COUNT > 4 && TASK_COUNT < 8) {
        TASK_COUNT = 4;
    }
    else if (TASK_COUNT > 8 && TASK_COUNT < 16) {
        TASK_COUNT = 8;
    } 
    else if (TASK_COUNT > 16) {
        TASK_COUNT = 16;
    }
    // now, TASK_COUNT is one of {2, 4, 8, 16}

    std::vector<std::shared_ptr<std::thread> > load_tasks;
    load_tasks.resize(TASK_COUNT);

    for (int task_id = TASK_COUNT - 1; task_id >= 0; --task_id) {
        load_tasks[task_id] = std::make_shared<std::thread>(
            [task_id, TASK_COUNT, begin, end, &pred, &load_tasks]()
        {
            auto count = std::distance(begin, end);
            size_t i_start = (count / TASK_COUNT) * task_id;

            auto it_start = std::next(begin, i_start);
            auto it_end = (task_id == TASK_COUNT - 1) ? end : std::next(it_start, count / TASK_COUNT);
            std::sort(it_start, it_end, pred);

            if ((task_id & 0x1) == 0) {
                //     0         1        2         3
                // =========----------==========----------
                // task 0 and 2 also need to merge
                load_tasks[task_id + 1]->join(); // the reason why counting down task_id

                auto it_mid = it_end;
                auto it_last = (task_id == TASK_COUNT - 2) ? end : std::next(it_mid, count / TASK_COUNT);
                std::inplace_merge(it_start, it_mid, it_last, pred);
            }
        });
    }

    for (auto &t : load_tasks) {
        if (t->joinable()) {
            t->join();
        }
    }

    const auto ASYNC_THRESH_HOLD = 40 * 1024;
    if (TASK_COUNT == 4) {
        std::inplace_merge(begin, std::next(begin, (count / TASK_COUNT) * 2), end, pred);
    }
    else if (TASK_COUNT == 8) {
        const auto step = count / TASK_COUNT;
        auto it2 = std::next(begin, step * 2);
        auto it4 = std::next(it2, step * 2);
        auto it6 = std::next(it4, step * 2);

        if (count > ASYNC_THRESH_HOLD) {
            auto fut = std::async(std::inplace_merge<Iter, Pr>, it4, it6, end, pred);
            std::inplace_merge(begin, it2, it4, pred);
            fut.get();
        }
        else {
            std::inplace_merge(begin, it2, it4, pred);
            std::inplace_merge(it4, it6, end, pred);
        }

        std::inplace_merge(begin, it4, end, pred);
    }
    else if (TASK_COUNT == 16) {
        const auto step = count / TASK_COUNT;
        auto it2 = std::next(begin, step * 2);
        auto it4 = std::next(it2, step * 2);
        auto it6 = std::next(it4, step * 2);
        auto it8 = std::next(it6, step * 2);
        auto it10 = std::next(it8, step * 2);
        auto it12 = std::next(it10, step * 2);
        auto it14 = std::next(it12, step * 2);

        auto merge_left_half = [&]() {
            std::inplace_merge(begin, it2, it4, pred);
            std::inplace_merge(it4, it6, it8, pred);
            std::inplace_merge(begin, it4, it8, pred);
        };
        auto merge_right_half = [&]() {
            std::inplace_merge(it8, it10, it12, pred);
            std::inplace_merge(it12, it14, end, pred);
            std::inplace_merge(it8, it12, end, pred);
        };

        if (count > ASYNC_THRESH_HOLD) {
            auto fut = std::async(merge_right_half);
            merge_left_half();
            fut.get();
        }
        else {
            merge_left_half();
            merge_right_half();
        }

        std::inplace_merge(begin, it8, end, pred);
    }
}

template<class Iter, class Pr> inline
void ParSortImpl2(Iter begin, Iter end, Pr pred, int TASK_COUNT)
{
    auto count = std::distance(begin, end);
    if (TASK_COUNT <= 1 || count <= 1024 * 2) { // threshold
        std::sort(begin, end, pred);
        return;
    }

    Iter mid = std::next(begin, count / 2);
    if (TASK_COUNT > 1) {
        auto fut = std::async(ParSortImpl2<Iter, Pr>, begin, mid, pred, TASK_COUNT - 2);
        ParSortImpl2(mid, end, pred, TASK_COUNT - 2);
        fut.get();
    }
    else {
        ParSortImpl2(begin, mid, pred, 0);
        ParSortImpl2(mid, end, pred, 0);
    }

    std::inplace_merge(begin, mid, end, pred);
}


template<class Iter, class Pr>
void ParSort(Iter begin, Iter end, Pr pred, int TASK_COUNT = 0)
{
    if (TASK_COUNT == 0) {
        TASK_COUNT = (decltype(TASK_COUNT))std::thread::hardware_concurrency();
    }
    if (TASK_COUNT <= 1) {
        std::sort(begin, end, pred);
        return;
    }

    ParSortImpl1(begin, end, pred, TASK_COUNT);
}

}
#endif //_SIMPLE_PAR_ALGORITHM_H
