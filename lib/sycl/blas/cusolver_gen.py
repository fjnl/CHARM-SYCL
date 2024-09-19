import blas_config as cfg
import sys
from template import Template

T = """
#include <blas/descs.hpp>
#include <cuda/context.hpp>
#include <format.hpp>
#include <blas/cusolver_interface.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace blas {

template <class SOL>
void clear_cusolver_lapack() {
{% for func in cfg.select_cusolver() %}
    {% for p in func.prefixes %}
        {{func.desc(p)}}.cuda = nullptr;
        {{func.desc(p)}}.cuda_tag = nullptr;
    {% end %}
{% end %}
}

template <class SOL>
error::result<std::string> init_cusolver_lapack() {
    using CUDA = typename SOL::CUDA;
    using BLAS = typename SOL::BLAS;

    auto ok = SOL::init();

    if (!ok) {
        clear_cusolver_lapack<SOL>();
        return error::unexpected(std::move(ok).error());
    }

{% for func in cfg.select_cusolver() %}
    {% for p in func.prefixes %}
    {{func.desc(p)}}.cuda = [](void** args) {
        {% for v in func.vars(p) %}
        {{v}};
        {% end %}
        
        auto err = SOL::{{func.buffer_fn()}}(
            {{ ', '.join(func.buffer_params(p)) }}
        );
        if (err) {
            FATAL("cuSOLVER Error: {}: err={}", "{{func.buffer_fn()}}", static_cast<int>(*err));
        }
        
        err = SOL::{{func.fn()}}(
            {{ ', '.join(func.params(p)) }}
        );
        if (err) {
            FATAL("cuSOLVER Error: {}: err={}", "{{func.fn()}}", static_cast<int>(*err));
        }
    };
    {{func.desc(p)}}.cuda_tag = &cusolver_tag;
    {% end %}
{% end %}

    return SOL::version();
}

template error::result<std::string> init_cusolver_lapack<runtime::cusolver_interface_11000>();
template void clear_cusolver_lapack<runtime::cusolver_interface_11000>();

}
CHARM_SYCL_END_NAMESPACE
""".lstrip()


class Function(cfg.Function):
    def buffer_fn(self):
        return self.fn() + "_buffer_size"

    def fn(self):
        return self.cusolver[0].replace("cusolverDn", "cusolver_dn_").lower()

    def params(self, dtype):
        return self.buffer_params(dtype)[:-2] + [
            "ws->d_buffer.get(d_byte)",
            "d_byte",
            "ws->h_buffer.get(h_byte)",
            "h_byte",
            "ws->d_info.get()",
        ]

    def buffer_params(self, dtype):
        result = ["ws->dn_handle.get()", "ws->dn_params.get()"]
        prec = (
            "CUDA::"
            + dict(s="k_R_32F", d="k_R_64F", c="k_C_32F", z="k_C_64F")[dtype.str]
        )

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
                result.append(prec)
                result.append(f"{arg.name} + {arg.name}_off")
            elif arg.is_size:
                result.append(arg.name)
            elif arg.is_dim:
                result.append(arg.name)

        if "prec" in self.cusolver:
            result.append(prec)
        if "workspace" in self.cusolver:
            result.append("&d_byte")
            result.append("&h_byte")

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
                tpe = "int64_t"
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

        result.append(
            f"auto ws = {ptr('runtime::cuda_context<CUDA, BLAS, SOL>', index)}"
        )
        if "workspace" in self.cusolver:
            result.append("size_t d_byte = 0")
            result.append("size_t h_byte = 0")

        return result


output = sys.argv[1]
input = sys.argv[2]
cfg = cfg.read_blas_ini(input, fn_cls=Function)
Template(T).save(output, {}, cfg=cfg)
