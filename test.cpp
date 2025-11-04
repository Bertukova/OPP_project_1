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

// Тест 1: Только один курильщик за раунд
TEST_F(SmokingTableTest, OnlyOneSmokerPerRound) {
    std::atomic<int> active_smokers{0};
    std::atomic<bool> test_running{true};
    
    auto smoker_task = [&](Ingredient ingredient) {
        if (table->startSmoking(ingredient)) {
            active_smokers++;
            while (test_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            table->finishSmoking();
        }
    };
    
    std::thread smoker1(smoker_task, Ingredient::kTobacco);
    std::thread smoker2(smoker_task, Ingredient::kPaper);
    std::thread smoker3(smoker_task, Ingredient::kMatches);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    table->place(Ingredient::kPaper, Ingredient::kMatches);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_EQ(active_smokers.load(), 1);
    
    test_running = false;
    table->finish();
    smoker1.join();
    smoker2.join();
    smoker3.join();
}

