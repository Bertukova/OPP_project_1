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

// Тест 2: Корректное завершение работы
TEST_F(SmokingTableTest, ProperShutdown) {
    std::atomic<int> completed_smokers{0};
    
    auto smoker_task = [&](Ingredient ingredient) {
        while (table->startSmoking(ingredient)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            table->finishSmoking();
        }
        completed_smokers++;
    };
    
    std::thread smoker1(smoker_task, Ingredient::kTobacco);
    std::thread smoker2(smoker_task, Ingredient::kPaper);
    std::thread smoker3(smoker_task, Ingredient::kMatches);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    table->finish();
    
    smoker1.join();
    smoker2.join();
    smoker3.join();
    
    EXPECT_EQ(completed_smokers.load(), 3);
}
// Тест 3: Многократные раунды
TEST_F(SmokingTableTest, MultipleRounds) {
    std::atomic<int> rounds_completed{0};
    std::atomic<bool> running{true};
    
    auto smoker_task = [&](Ingredient ingredient) {
        while (running) {
            if (table->startSmoking(ingredient)) {
                rounds_completed++;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                table->finishSmoking();
            }
        }
    };
    
    std::thread smoker1(smoker_task, Ingredient::kTobacco);
    std::thread smoker2(smoker_task, Ingredient::kPaper);
    std::thread smoker3(smoker_task, Ingredient::kMatches);
    
    // Проводим 5 раундов
    for (int i = 0; i < 5; i++) {
        table->place(Ingredient::kPaper, Ingredient::kMatches);
        table->waitForRoundEnd();
    }
    
    running = false;
    table->finish();
    smoker1.join();
    smoker2.join();
    smoker3.join();
    
    EXPECT_GE(rounds_completed.load(), 5);
}
// Тест 4: Все три курильщика работают
TEST_F(SmokingTableTest, AllSmokersParticipate) {
    std::array<std::atomic<int>, 3> smoker_counts{0, 0, 0};
    std::atomic<bool> running{true};
    
    auto smoker_task = [&](Ingredient ingredient, int index) {
        while (running) {
            if (table->startSmoking(ingredient)) {
                smoker_counts[index]++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                table->finishSmoking();
            }
        }
    };
    
    std::thread smoker1(smoker_task, Ingredient::kTobacco, 0);
    std::thread smoker2(smoker_task, Ingredient::kPaper, 1);
    std::thread smoker3(smoker_task, Ingredient::kMatches, 2);
    
    // Разные комбинации компонентов
    table->place(Ingredient::kPaper, Ingredient::kMatches);  // Для табака
    table->waitForRoundEnd();
    table->place(Ingredient::kTobacco, Ingredient::kMatches); // Для бумаги
    table->waitForRoundEnd();
    table->place(Ingredient::kTobacco, Ingredient::kPaper);   // Для спичек
    table->waitForRoundEnd();
    
    running = false;
    table->finish();
    smoker1.join();
    smoker2.join();
    smoker3.join();
    
    // Все курильщики должны были покурить
    EXPECT_GT(smoker_counts[0].load(), 0);
    EXPECT_GT(smoker_counts[1].load(), 0);
    EXPECT_GT(smoker_counts[2].load(), 0);
}






