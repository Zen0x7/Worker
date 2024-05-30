#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <vector>
#include <string>
#include <shared_mutex>
#include <iostream>

namespace queue {
    class transaction {
    public:
        std::time_t created_at_ { std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
        std::time_t updated_at { std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
        std::string action_;
        enum statusses {
            PENDING,
            SENT,
            PROCESSED,
        } status_ = PENDING;
    };

    inline std::unordered_map<std::string, transaction> transactions;
    inline std::mutex lock;

    inline void push_back(std::string const &transaction_id, std::string const &action) {
        std::lock_guard scoped_lock(lock);

        transactions.insert({ transaction_id, { .action_ = action }});
    }

    inline void change_status(std::string const &transaction_id, transaction::statusses const status) {
        using std::chrono::system_clock;
        std::lock_guard scoped_lock(lock);

        transactions[transaction_id].status_ = status;
        transactions[transaction_id].updated_at = system_clock::to_time_t(system_clock::now());
    }
};

#endif // QUEUE_HPP
