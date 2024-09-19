#include "task.hpp"
#include <cassert>
#include <mutex>
#include <charm/sycl/config.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace dev_rts {

void task::runs_after(task_ptr const& dependee) {
    std::unique_lock lk(dependee->m_);

    if (!dependee->done_) {
        std::unique_lock lk(m_);

        dependee->nexts_.push_back(shared_from_this());
        n_wait_ += 1;
    }
}

void task::finalize() {
    notify();
}

void task::init() {
    nexts_.reserve(8);
}

void task::notify() {
    std::unique_lock lk(m_);

    n_wake_ += 1;
    if (n_wait_ == n_wake_) {
        prepare(std::move(lk));
    }
}

void task::prepare(std::unique_lock<std::mutex> lk) {
    run_ = true;

    if (is_nop()) {
        complete(std::move(lk));
    } else {
        run(std::move(lk));
    }
}

void task::run(std::unique_lock<std::mutex>) {
    q_.push(shared_from_this());
}

void task::complete() {
    complete(std::unique_lock(m_));
}

void task::complete(std::unique_lock<std::mutex>) {
    done_ = true;

    for (auto& next : nexts_) {
        next->notify();
    }
}

}  // namespace dev_rts
CHARM_SYCL_END_NAMESPACE
