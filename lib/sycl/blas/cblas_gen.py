import sys
from template import Template
import blas_config as cfg

T = """
#include <type_traits>
#include <blas/descs.hpp>
#include <blas/{{ns}}_interface_32.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace blas {

template <class {{T}}>
void clear_{{ns}}{{suffix}}() {
{% for func in cfg.functions %}
    {% for p in func.prefixes %}
    {{func.desc(p)}}.cpu = nullptr;
    {{func.desc(p)}}.cpu_tag = nullptr;
    {% end %}
{% end %}
}

template <class {{T}}>
error::result<std::string> init_{{ns}}{{suffix}}(void* h) {
    auto ok = [&](){
        if constexpr (std::is_invocable_v<decltype({{T}}::init), void*>) {
            return {{T}}::init(h);
        } else {
            return {{T}}::init();
        }
    }();

    if (!ok) {
        clear_{{ns}}{{suffix}}<{{T}}>();
        return error::unexpected(std::move(ok).error());
    }

{% for func in cfg.functions %}
    {% for p in func.prefixes %}
    {{func.desc(p)}}.cpu = [](void** args) {
        {% for v in func.vars(p) %}
        {{v}};
        {% end %}

        {{T}}::{{func.fn(p)}}(
            {% if func.needs_layout() %}
            {{into.replace('typename ', '')}}::COL_MAJOR,
            {% end %}
            {{ ', '.join(func.params(p)) }}
        );
    };
    {{func.desc(p)}}.cpu_tag = &{{ns}}_tag;
    {% end %}
{% end %}

    return {{T}}::version();
}

template error::result<std::string> init_{{ns}}{{suffix}}<runtime::{{ns}}_interface_32>(void*);
template void clear_{{ns}}{{suffix}}<runtime::{{ns}}_interface_32>();

}
CHARM_SYCL_END_NAMESPACE
""".lstrip()


class Function(cfg.Function):
    def fn(self, dtype):
        global ns
        return f"{ns}_{self.name(dtype).lower()}"

    def needs_layout(self):
        for arg in self.args:
            if arg.is_matrix:
                return True
        return False

    def params(self, dtype):
        global into
        result = super().params()

        for i, param in enumerate(result):
            if "_to_int" in param:
                if ns == "lapacke":
                    result[i] = param.replace("_to_int", f"_to_char")
                else:
                    result[i] = param.replace("_to_int", f"_into<{into}>")
            if self.args[i].is_scalar and dtype.is_complex():
                result[i] = "&" + result[i]
        return result


if sys.argv[1] == "lapacke":
    suffix = ""
    ns = "lapacke"
    t = "LAPACK"
    into = "typename LAPACK::BLAS"
else:
    suffix = int(sys.argv[1])
    ns = "cblas"
    t = "BLAS"
    into = t
output = sys.argv[2]
input = sys.argv[3]
cfg = cfg.read_blas_ini(input, fn_cls=Function)
Template(T).save(output, {}, cfg=cfg, suffix=suffix, T=t, ns=ns, into=into)
