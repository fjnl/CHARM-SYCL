#include "common.hpp"

TEST_CASE("parallel_for", "") {
    sycl::queue q;

    q.submit([&](sycl::handler& h) {
         h.parallel_for(sycl::range(1), [=](sycl::id<1>) {});
     }).wait();

    q.submit([&](sycl::handler& h) {
         h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {});
     }).wait();

    q.submit([&](sycl::handler& h) {
         h.parallel_for(sycl::range(1, 1), [=](sycl::id<2>) {});
     }).wait();

    q.submit([&](sycl::handler& h) {
         h.parallel_for(sycl::range(1, 1), [=](sycl::id<2> const&) {});
     }).wait();

    q.submit([&](sycl::handler& h) {
         h.parallel_for(sycl::range(1, 1, 1), [=](sycl::id<3>) {});
     }).wait();

    q.submit([&](sycl::handler& h) {
         h.parallel_for(sycl::range(1, 1, 1), [=](sycl::id<3> const&) {});
     }).wait();

    SUCCEED();
}
