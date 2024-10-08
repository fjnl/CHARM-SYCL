#include "kreg.hpp"
#include <cstring>
#include <unordered_set>

namespace {

using namespace kreg;

template <class V>
struct hval {
    hval() = default;

    explicit hval(std::string_view k, uint32_t h) : hash(h), key(k), val() {}

    template <class... Args>
    explicit hval(std::string_view k, uint32_t h, Args&&... args)
        : hash(h), key(k), val(std::make_unique<V>(std::forward<Args>(args)...)) {}

    void construct() {
        val = std::make_unique<V>();
    }

    friend inline bool operator==(hval<V> const& lhs, hval<V> const& rhs) {
        return lhs.hash == rhs.hash && lhs.key == rhs.key;
    }

    struct hash_fn {
        inline auto operator()(hval<V> const& hv) const {
            return hv.hash;
        }
    };

    uint32_t hash;
    std::string key;
    std::unique_ptr<V> val;
};

struct kernel_registry_impl final : kreg::kernel_registry {
    void add(std::string_view name, uint32_t name_hash, std::string_view kind,
             uint32_t kind_hash, void* f, int is_ndr) override {
        auto& storage = get(kind, kind_hash);

        hval<kernel_info> hv(name, name_hash, f, is_ndr);

        storage.insert(std::move(hv));
    }

    kernel_info const* find(std::string_view name, uint32_t name_hash, std::string_view kind,
                            uint32_t kind_hash) override {
        auto& storage = get(kind, kind_hash);
        hval<kernel_info> hv(name, name_hash);

        if (auto it = storage.find(hv); it != storage.end()) {
            return it->val.get();
        }
        return nullptr;
    }

private:
    template <class V>
    using hset = std::unordered_set<hval<V>, typename hval<V>::hash_fn>;

    hset<kernel_info>& get(std::string_view kind, uint32_t hash) {
        hval<hset<kernel_info>> hv(kind, hash);
        if (auto it = data_.find(hv); it != data_.end()) {
            return *it->val;
        }

        hv.construct();

        auto res = data_.insert(std::move(hv));
        return *res.first->val;
    }

    hset<hset<kernel_info>> data_;
};

}  // namespace

namespace kreg {

kernel_registry& get() {
    static kernel_registry_impl inst;
    return inst;
}

}  // namespace kreg

extern "C" void __s_add_kernel_registry(char const* name, unsigned long name_hash,
                                        char const* kind, unsigned long kind_hash, void* f,
                                        int is_ndr) {
    static_assert(sizeof(unsigned long) >= 4);
    kreg::get().add(name, name_hash, kind, kind_hash, f, is_ndr);
}
