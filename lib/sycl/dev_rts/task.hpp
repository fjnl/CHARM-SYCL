#pragma once

#include <memory>
#include <mutex>
#include <BS_thread_pool.hpp>
#include <charm/sycl/config.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace dev_rts {

struct task;

using task_ptr = std::shared_ptr<task>;
using task_weak_ptr = std::weak_ptr<task>;

struct queue {
    queue() : q_(1) {}

    ~queue() {
        wait_all();
    }

    template <class Task>
    void push(std::shared_ptr<Task> const& task) {
        q_.detach_task([task]() {
            task->run_op();
        });
    }

    void wait_all() {
        q_.wait();
    }

private:
    BS::thread_pool q_;
};

struct op_base {
    virtual ~op_base() = default;

    virtual void call(task_ptr const&) = 0;
};

using op_ptr = std::unique_ptr<op_base>;

struct task : std::enable_shared_from_this<task> {
    task() : op_() {
        init();
    }

    explicit task(op_ptr&& op) : op_(std::move(op)) {
        init();
    }

    void override_op(op_ptr&& op) {
        op_ = std::move(op);
    }

    virtual ~task() = default;

    void runs_after(task_ptr const& dependee);

    void finalize();

    void complete();

private:
    friend struct queue;
    static inline queue q_;

    void init();

    void notify();

    void prepare(std::unique_lock<std::mutex> lk);

    void run(std::unique_lock<std::mutex> lk);

    void complete(std::unique_lock<std::mutex> lk);

    void run_op() {
        op_->call(shared_from_this());
    }

    bool is_nop() const {
        return op_ == nullptr;
    }

    std::mutex m_;
    op_ptr op_;
    std::vector<task_ptr> nexts_;
    uint16_t n_wake_ = 0;
    uint16_t n_wait_ = 1;
    bool done_ = false;
    bool run_ = false;
};

inline task_ptr make_nop_task() {
    return std::make_shared<task>();
}

inline task_ptr make_op_task(op_ptr&& op) {
    return std::make_shared<task>(std::move(op));
}

template <class Op>
inline task_ptr make_task_from_op(Op&& op) {
    return make_op_task(std::make_unique<Op>(std::forward<Op>(op)));
}

}  // namespace dev_rts
CHARM_SYCL_END_NAMESPACE
