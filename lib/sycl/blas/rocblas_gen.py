import sys
from template import Template
import blas_config as cfg

T = """
#include <blas/rocblas_interface.hpp>
#include <blas/descs.hpp>
#include <format.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace blas {

template <class BLAS>
void clear_rocblas{{level}}() {
{% for func in cfg.functions %}
    {% for p in func.prefixes %}
    {{func.desc(p)}}.hip = nullptr;
    {{func.desc(p)}}.hip_tag = nullptr;
    {% end %}
{% end %}
}

template <class BLAS>
error::result<std::string> init_rocblas{{level}}() {
    CHECK_ERROR(BLAS::init());

{% for func in cfg.functions %}
    {% for p in func.prefixes %}
    {{func.desc(p)}}.hip = [](void** args) {
        {% for v in func.vars(p) %}
        {{v}};
        {% end %}

        auto err = BLAS::{{func.fn(p)}}(
            {{ ', '.join(func.params()) }}
        );

        if (err) {
            FATAL("rocBLAS Error: {}: {}", "{{func.fn(p)}}", BLAS::rocblas_status_to_string(err));
        }
    };
    {{func.desc(p)}}.hip_tag = &rocblas_tag;
    {% end %}
{% end %}

    return BLAS::version();
}

template error::result<std::string> init_rocblas{{level}}<runtime::rocblas_interface_40200000>();
template void clear_rocblas{{level}}<runtime::rocblas_interface_40200000>();

}
CHARM_SYCL_END_NAMESPACE
""".lstrip()


class Function(cfg.Function):
    def fn(self, dtype):
        return f"rocblas_{self.name(dtype).lower()}"

    def vars(self, dtype):
        result = super().vars(dtype)
        result.append(
            "auto ctx = " + self.cast_arg("typename BLAS::handle_t", self.nparams())
        )
        return result

    def params(self):
        result = super().params()
        scalar = set()

        for i, arg in enumerate(self.args):
            if arg.is_scalar:
                scalar.add(i)

        for i, param in enumerate(result):
            if "_to_int" in param:
                result[i] = param.replace("_to_int", "_into<BLAS>")
            if i in scalar:
                result[i] = "&" + result[i]
        result = ["ctx"] + result
        return result


level = int(sys.argv[1])
output = sys.argv[2]
input = sys.argv[3]
cfg = cfg.read_blas_ini(input, fn_cls=Function)
Template(T).save(output, {}, cfg=cfg, level=level)
