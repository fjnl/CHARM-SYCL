#include "dev_rts.hpp"

namespace dev_rts {

std::unique_ptr<BS::thread_pool> q_task;
memory_domain_impl g_dom;
std::chrono::high_resolution_clock::time_point t0;

}  // namespace dev_rts
