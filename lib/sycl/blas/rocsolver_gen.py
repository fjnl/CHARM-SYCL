import blas_config as cfg
import sys
from template import Template

T = """
#include <blas/rocsolver_interface.hpp>
#include <blas/descs.hpp>
#include <format.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace blas {

template <class SOL>
void clear_rocsolver_lapack() {
{% for func in cfg.select_rocsolver() %}
    {% for dtype in func.prefixes %}
        {{func.desc(dtype)}}.hip = nullptr;
        {{func.desc(dtype)}}.hip_tag = nullptr;
    {% end %}
{% end %}
}

template <class SOL>
error::result<std::string> init_rocsolver_lapack() {
    using BLAS = typename SOL::BLAS;

    CHECK_ERROR(SOL::init());

{% for func in cfg.select_rocsolver() %}
    {% for dtype in func.prefixes %}
    {{func.desc(dtype)}}.hip = [](void** args) {
        {% for v in func.vars(dtype) %}
        {{v}};
        {% end %}

        auto err = SOL::{{func.fn(dtype)}}(
            {{ ', '.join(func.params(dtype)) }}
        );
        if (err) {
            FATAL("rocSOLVER Error: {}: {}", "{{func.fn(dtype)}}", BLAS::rocblas_status_to_string(err));
        }
    };
    {{func.desc(dtype)}}.hip_tag = &rocsolver_tag;
    {% end %}
{% end %}

    return SOL::version();
}

template error::result<std::string> init_rocsolver_lapack<runtime::rocsolver_interface_40200000>();
template void clear_rocsolver_lapack<runtime::rocsolver_interface_40200000>();

}
CHARM_SYCL_END_NAMESPACE
""".lstrip()


class Function(cfg.Function):
    def buffer_fn(self):
        return self.fn() + "_buffer_size"

    def fn(self, dtype):
        return "rocsolver_" + self.name(dtype)

    def params(self, dtype):
        result = ["ctx"]

        for arg in self.args:
            if arg.is_trans:
                result.append(f"trans_into<BLAS>({arg.name})")
            elif arg.is_uplo:
                result.append(f"uplo_into<BLAS>({arg.name})")
            elif arg.is_diag:
                result.append(f"diag_into<BLAS>({arg.name})")
            elif arg.is_side:
                result.append(f"side_into<BLAS>({arg.name})")
            elif arg.is_scalar:
                result.append(arg.name)
            elif arg.is_vector or arg.is_matrix:
                result.append(f"{arg.name} + {arg.name}_off")
            elif arg.is_size:
                result.append(arg.name)
            elif arg.is_dim:
                result.append(arg.name)
        result.append("info")

        return result

    def vars(self, dtype):
        def ptr(tpe, idx):
            return f"reinterpret_cast<{tpe}*>(args[{idx}])"

        def cast(tpe, idx):
            return f"(*({ptr(tpe, idx)}))"

        result = []
        index = 0
        for arg in self.args:
            expr = None
            if arg.is_trans:
                tpe = "blas::trans"
            elif arg.is_uplo:
                tpe = "blas::uplo"
            elif arg.is_diag:
                tpe = "blas::diag"
            elif arg.is_side:
                tpe = "blas::side"
            elif arg.is_scalar:
                tpe = dtype.scalar_type
            elif arg.is_vector or arg.is_matrix:
                tpe = dtype.array_type
            elif arg.is_size:
                tpe = "int32_t"
            elif arg.is_dim:
                tpe = "rts::accessor"
                expr = cast(tpe, index) + ".size[2]"

            if expr is None:
                expr = cast(tpe, index)
            result.append(f"auto {arg.name} = {expr}")
            index += 1

            if arg.is_vector or arg.is_matrix:
                expr = "rts::acc_linear_off(" + cast("rts::accessor", index) + ")"
                result.append(f"auto {arg.name}_off = {expr}")

        result.append(f"auto ctx = {cast('typename SOL::handle_t', index)}")
        result.append(f"auto info = {ptr('int', index+1)}")
        return result


output = sys.argv[1]
input = sys.argv[2]
cfg = cfg.read_blas_ini(input, fn_cls=Function)
Template(T).save(output, {}, cfg=cfg)
