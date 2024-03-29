

#include <sccl.h>

#include <gtest/gtest.h>

TEST(nccl_instance, create_instance)
{

    sccl_instance_t instance;
    EXPECT_EQ(sccl_create_instance(&instance), sccl_success);

    sccl_destroy_instance(instance);
}
