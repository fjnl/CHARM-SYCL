#include <random>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "common.hpp"

struct particle {
    double x, y, z;
    double vx, vy, vz;
    double mass;
};

static inline auto distance(particle const& p, particle const& q) {
    return sycl::sqrt((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y) +
                      (p.z - q.z) * (p.z - q.z));
}

static constexpr unsigned N = 1000;
static constexpr double G = 6.67;
static constexpr double dt = 1.0e-3;

static std::tuple<double, double, double> center_of(std::vector<particle> const& in) {
    auto const n = in.size();
    double cx = 0, cy = 0, cz = 0;
    for (size_t i = 0; i < n; i++) {
        auto const& p_i = in[i];
        cx += p_i.x;
        cy += p_i.y;
        cz += p_i.z;
    }
    return std::make_tuple(cx / n, cy / n, cz / n);
}

// static void verify(std::vector<particle> const& in) {
//     auto const n = in.size();

//     {
//         auto const [cx, cy, cz] = center_of(in);
//         printf("c=(%f, %f, %f)\n", cx, cy, cz);
//     }

//     double cx = 0, cy = 0, cz = 0;
//     for (size_t i = 0; i < n; i++) {
//         auto const& p_i = in[i];
//         double dx = 0, dy = 0, dz = 0;

//         for (size_t j = 0; j < n; j++) {
//             if (i != j) {
//                 auto const& p_j = in[j];
//                 auto const r = distance(p_i, p_j);

//                 dx += G * (p_j.mass / (r * r)) * ((p_j.x - p_i.x) / r);
//                 dy += G * (p_j.mass / (r * r)) * ((p_j.y - p_i.y) / r);
//                 dz += G * (p_j.mass / (r * r)) * ((p_j.z - p_i.z) / r);
//             }
//         }

//         auto const vx = p_i.vx + dt * dx;
//         auto const vy = p_i.vy + dt * dy;
//         auto const vz = p_i.vz + dt * dz;
//         cx += p_i.x + dt * vx;
//         cy += p_i.y + dt * vy;
//         cz += p_i.z + dt * vz;
//     }

//     printf("c=(%f, %f, %f)\n", cx / n, cy / n, cz / n);
// }

TEST_CASE("nbody", "") {
    sycl::queue q;
    std::mt19937 g(1);
    std::vector<particle> init, result;

    for (size_t i = 0; i < N; i++) {
        particle p;
        p.x = std::uniform_real_distribution<double>(0.0, 10.0)(g);
        p.y = std::uniform_real_distribution<double>(0.0, 10.0)(g);
        p.z = std::uniform_real_distribution<double>(0.0, 10.0)(g);
        p.vx = std::uniform_real_distribution<double>(0.1, 10.0)(g);
        p.vy = std::uniform_real_distribution<double>(0.1, 10.0)(g);
        p.vz = std::uniform_real_distribution<double>(0.1, 10.0)(g);
        p.mass = std::uniform_real_distribution<double>(100, 200)(g);
        init.push_back(p);
    }
    result.resize(init.size());
    // verity(init);

    {
        sycl::buffer<particle, 1> in_buf(init.data(), {init.size()});
        sycl::buffer<particle, 1> out_buf(result.data(), {result.size()});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<particle, 1, sycl::access_mode::read> in(in_buf, h);
            sycl::accessor<particle, 1, sycl::access_mode::write> out(out_buf, h);

            h.parallel_for(sycl::range(N), [=](sycl::id<1> idx) {
                auto const i = idx[0];
                auto const p_i = in[i];
                double dx = 0, dy = 0, dz = 0;

                for (unsigned j = 0; j < N; j++) {
                    if (i != j) {
                        auto const p_j = in[j];
                        auto const r = distance(p_i, p_j);

                        dx += G * (p_j.mass / (r * r)) * ((p_j.x - p_i.x) / r);
                        dy += G * (p_j.mass / (r * r)) * ((p_j.y - p_i.y) / r);
                        dz += G * (p_j.mass / (r * r)) * ((p_j.z - p_i.z) / r);
                    }
                }

                auto const vx = p_i.vx + dt * dx;
                auto const vy = p_i.vy + dt * dy;
                auto const vz = p_i.vz + dt * dz;
                out[i].x = p_i.x + dt * vx;
                out[i].y = p_i.y + dt * vy;
                out[i].z = p_i.z + dt * vz;
                out[i].vx = vx;
                out[i].vy = vy;
                out[i].vz = vz;
                out[i].mass = p_i.mass;
            });
        });

        ev.wait();
    }

    auto const [cx, cy, cz] = center_of(result);
    REQUIRE_THAT(cx, Catch::Matchers::WithinRel(4.871594, 1e-6));
    REQUIRE_THAT(cy, Catch::Matchers::WithinRel(5.133879, 1e-6));
    REQUIRE_THAT(cz, Catch::Matchers::WithinRel(4.985096, 1e-6));
}
