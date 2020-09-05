//
// Created by fhq on 2020/9/6.
//

#include <gtest/gtest.h>
#include <string>
using std::string;


TEST(SalutationTest, Static) {
    EXPECT_EQ(string("Hello World!"), string("Hello World."));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}