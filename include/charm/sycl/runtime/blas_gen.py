import sys
import io
import blas_config as cfg
from template import Template

T = """
#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace blas {

{% for func in cfg.functions %}
    {% for dtype in func.prefixes %}
extern void const* const desc_{{func.name(dtype)}};
    {% end %}
{% end %}

{% for func in cfg.functions %}
    {% for dtype in func.prefixes %}
template <class AllocatorT>
auto {{func.name(dtype)}}(queue& q, {{','.join(func.sycl_params(dtype))}}) {
    return q.submit([&](auto& h) {
        runtime::impl_access::get_impl(h)->set_desc(desc_{{func.name(dtype)}});
    {% for x in func.read %}
        auto {{x.lower()}} = {{x}}.template get_access<access_mode::read>(h);
    {% end %}
    {% for x in func.readwrite %}
        auto {{x.lower()}} = {{x}}.get_access(h);
    {% end %}
    {% for x in func.write %}
        auto {{x.lower()}} = {{x}}.template get_access<access_mode::write>(h);
    {% end %}
        h.__bind({{','.join(func.sycl_params(dtype, True, True))}});
    });
}

template <class AllocatorT>
auto {{func.common_name}}(queue& q, {{','.join(func.sycl_params(dtype))}}) {
    return {{func.name(dtype)}}(q, {{','.join(func.sycl_params(dtype, True))}});
}

    {% end %}
{% end %}

}
CHARM_SYCL_END_NAMESPACE
"""


class Function(cfg.Function):
    def sycl_params(self, dtype, vars_only=False, lower_buffers=False):
        types = []
        vars = []
        for arg in self.args:
            need_lower = False
            skip = False

            if arg.is_trans:
                types.append("trans")
            elif arg.is_uplo or arg.is_diag or arg.is_side:
                types.append(arg.name.lower())
            elif arg.is_scalar:
                types.append(dtype.scalar_type)
            elif arg.is_matrix:
                types.append(f"buffer<{dtype.scalar_type}, 2, AllocatorT>&")
                need_lower = lower_buffers
            elif arg.is_vector:
                types.append(f"buffer<{dtype.scalar_type}, 1, AllocatorT>&")
                need_lower = lower_buffers
            elif arg.is_dim:
                skip = True
            else:
                types.append("std::size_t")

            if not skip:
                vars.append(arg.name.lower() if need_lower else arg.name)

        if vars_only:
            return vars
        return list(map(lambda p: " ".join(p), zip(types, vars)))


input = sys.argv[1]
output = sys.argv[2]
cfg = cfg.read_blas_ini(input, fn_cls=Function)
Template(T).save(output, {}, cfg=cfg)
