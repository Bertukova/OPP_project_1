#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

#include "smoking_types.hpp"
#include "smoking_table.hpp"
#include "smoking_io.hpp"

class SmokingTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        table = std::make_unique<SmokingTable>();
    }

    void TearDown() override {
        if (table) {
            table->finish();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::unique_ptr<SmokingTable> table;
    std::mutex test_mutex;
};



