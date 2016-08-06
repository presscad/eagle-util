// TestBoost.cpp : Defines the entry point for the console application.
//

#include <assert.h>
#include "boost/lambda/lambda.hpp"
#include <iostream>
#include <iterator>
#include <algorithm>

void lambda()
{
    using namespace boost::lambda;
    typedef std::istream_iterator<int> in;

    std::for_each(
        in(std::cin), in(), std::cout << (_1 * 3) << " " );
}

void TestConcurrentQueue();

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
void TestWeekPtr()
{
    boost::shared_ptr<char> sp(new char[100]);
    boost::shared_ptr<char> sp2(sp);
    strcpy(sp.get(), "100");

    boost::weak_ptr<char> wp(sp);
    sp.reset();
    assert(sp2.use_count() == 1);

    boost::shared_ptr<char> sp3 = wp.lock();
    assert(sp3.use_count() == 2);

    sp2.reset();
    sp3.reset();
    boost::shared_ptr<char> sp4 = wp.lock();
    assert(sp4.get() == NULL);
}

#include <boost/atomic.hpp>
void TestAtomic()
{
    boost::atomic<int> a(1);
    a++;
    assert(a == 2);
}

int main()
{
    TestWeekPtr();
    TestConcurrentQueue();
    TestAtomic();
}
