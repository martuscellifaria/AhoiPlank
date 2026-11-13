#include "IconProvider.h"
#include <gtest/gtest.h>
#include <memory>

TEST(BasicSetupTest, GoogleTestWorks) {
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
}

class IconProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
	test_instance_ = std::make_unique<IconProvider>();
    }
    void TearDown() override {
    }
public:
    std::unique_ptr<IconProvider> test_instance_;
};

