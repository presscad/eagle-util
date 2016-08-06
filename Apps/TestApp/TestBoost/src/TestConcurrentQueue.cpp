
#include <assert.h>
#include "ConcurrentQueue.h"

void TestConcurrentQueue()
{
    concurrent_queue<int> q;
    q.push(1);
    q.push(2);

    int v1, v2;
    q.try_pop(v1);
    q.wait_and_pop(v2);

    assert(q.empty());
}
