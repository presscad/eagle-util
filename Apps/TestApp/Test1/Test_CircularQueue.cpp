#include "stdafx.h"
#include <assert.h>
#include <vector>
#include "CircularQueue.h"

int Test_CircularQueue()
{
    circle_queue_t q;
    circle_queue_clear(&q, 10);

    // test push back
    for (int i=1; i<=6; i++)
    {
        shared_record_t r;
        r.value = i;

        circle_queue_push_back(&q, &r);
    }
    assert(circle_queue_get_size(&q) == 6);

    // test bulk get
    std::vector<p_shared_record_t> arr;
    arr.resize(circle_queue_get_size(&q));
    circle_queue_get_records(&q, arr.data(), arr.size());

    // more push back
    shared_record_t r;
    r.value = 7;
    circle_queue_push_back(&q, &r);
    r.value = 8;
    circle_queue_push_back(&q, &r);
    assert(circle_queue_get_size(&q) == 8);

    // bulk erase
    circle_queue_bulk_erase(&q, arr[0], arr.size());
    assert(circle_queue_get_size(&q) == 2);

    // more push back
    r.value = 9;
    circle_queue_push_back(&q, &r);
    r.value = 10;
    circle_queue_push_back(&q, &r);
    r.value = 11;
    circle_queue_push_back(&q, &r);
    r.value = 12;
    circle_queue_push_back(&q, &r);
    assert(circle_queue_get_size(&q) == 6);

    // bulk erase
    arr.resize(circle_queue_get_size(&q));
    circle_queue_get_records(&q, arr.data(), arr.size());
    r.value = 13;
    circle_queue_push_back(&q, &r);
    circle_queue_bulk_erase(&q, arr[1], arr.size() - 1);

    assert(circle_queue_get_size(&q) == 2);

    return 0;
}
