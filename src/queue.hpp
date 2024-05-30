#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <vector>
#include <string>
#include <shared_mutex>

namespace queue {
    class transaction {
    public:
        std::string id;
        std::string action;
    };

    inline std::vector<transaction> pending_transactions;
    inline std::mutex lock;

    inline void push_back(std::string const &transaction_id, std::string const &action) {
        std::lock_guard scoped_lock(lock);

        pending_transactions.push_back({
            .id = transaction_id,
            .action = action,
        });
    }

    inline void erase(std::string const &transaction_id) {
        std::lock_guard scoped_lock(lock);

        if (const auto it = std::find_if(
            pending_transactions.begin(),
            pending_transactions.end(),
            [transaction_id](transaction &item) { return item.id == transaction_id; }
        ); it != pending_transactions.end()) {
            pending_transactions.erase(it);
        }
    }
};

#endif // QUEUE_HPP
