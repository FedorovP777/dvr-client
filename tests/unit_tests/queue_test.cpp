#include <gtest/gtest.h>
#include "../../src/SharedQueue.h"
namespace
{

TEST(SharedQueueTest, BasicAssertions)
{
    SharedQueue<int *> sq;
    EXPECT_EQ(0, sq.getSize());
    auto *insert_value = new int(1);
    sq.push(insert_value);
    EXPECT_EQ(1, sq.getSize());
    auto *pop_value = sq.popIfExist();
    EXPECT_EQ(0, sq.getSize());
    EXPECT_EQ(insert_value, pop_value);
}
}
