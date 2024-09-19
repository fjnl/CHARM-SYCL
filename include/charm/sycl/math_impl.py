from __future__ import print_function
from vec_gen import *


def params(type, var, n):
    return ",".join([type + " " + var + str(i) for i in range(n)])


def args(var, n):
    return ",".join([var + str(i) for i in range(n)])


def scalar(types, name, n_args):
    return "\n".join(
        render(
            "$ty __charm_sycl_${name}_${sig}(${params}){ return std::${fn}(${args}); }",
            ty=ty,
            name=name,
            sig=sigs[ty],
            fn=("pow" if name == "powr" else name),
            params=params(ty, "x", n_args),
            args=args("x", n_args),
        )
        for ty in types
    )


def vector(types, name, n_args, veclen=None, reduce=False):
    if not veclen:
        veclen = ns
    sigs_ = [render("v${n}${t}", n=n, t=sigs[ty]) for ty in types for n in veclen]
    types = [
        render("sycl::vec<${ty}, ${n}>", ty=ty, n=n) for ty in types for n in veclen
    ]
    res = ""
    fn = "pow" if name == "powr" else name

    for ty, sig in zip(types, sigs_):
        vl = int(sig[1:-1])
        body = "{"
        for v in range(vl):
            if v > 0:
                body += ", "
            body += "std::" + fn + "("
            body += ",".join("x{i}[{v}]".format(i=i, v=v) for i in range(n_args))
            body += ")"
        body += "}"

        res += render(
            "${ret} __charm_sycl_${name}_${sig}(${params}){ return ${body}; }",
            ret=(
                "typename " + ty + "::element_type"
                if reduce
                else "typename " + ty + "::vector_t"
            ),
            name=name,
            sig=sig,
            params=params("typename " + ty + "::vector_t", "x", n_args),
            body=body,
        )
    return res


host = ""

for fn in math_funcs:
    name = fn.split(":")[0]

    if name in ["distance", "length", "clamp"]:
        continue

    if fn.endswith(":gfgf"):
        host += scalar(floats, name, 2)
    elif fn.endswith(":gigi"):
        host += scalar(ints, name, 2)
    elif fn.endswith(":gf"):
        host += scalar(floats, name, 1)
    elif fn.endswith(":gi"):
        host += scalar(floats, name, 1)
    else:
        assert False, fn

element_wise = ""

for fn in math_funcs:
    name = fn.split(":")[0]

    if name in ["distance", "length", "clamp"]:
        continue

    float_only = False
    int_only = False

    if fn.endswith(":gfgf"):
        float_only = True
        n_args = 2
    elif fn.endswith(":gigi"):
        int_only = True
        n_args = 2
    elif fn.endswith(":gf"):
        float_only = True
        n_args = 1
    elif fn.endswith(":gi"):
        int_only = True
        n_args = 1
    else:
        assert False, fn

    if float_only:
        types = floats
    elif int_only:
        types = ints

    for ty in types:
        for vl in ns:
            t = sigs[ty]
            vec_t = "v{vl}{t}_t".format(vl=vl, t=t)

            if n_args == 1:
                exprs = ",".join(
                    "__charm_sycl_{name}_{t}(__charm_sycl_vec_ix_v{vl}{t}(x0, {v}))".format(
                        v=v, vl=vl, name=name, t=t
                    )
                    for v in range(vl)
                )
            elif n_args == 2:
                exprs = ",".join(
                    "__charm_sycl_{name}_{t}(__charm_sycl_vec_ix_v{vl}{t}(x0, {v}), __charm_sycl_vec_ix_v{vl}{t}(x1, {v}))".format(
                        v=v, vl=vl, name=name, t=t
                    )
                    for v in range(vl)
                )

            element_wise += render(
                """
                ${vec_t} __charm_sycl_${name}_v${vl}${t}(
                    ${arg1} ${arg2}
                ) {
                    return __charm_sycl_vec_v${vl}${t}(
                        ${exprs}
                    );
                }
                """,
                arg1=(vec_t + " x0" if n_args > 0 else ""),
                arg2=("," + vec_t + " x1" if n_args > 1 else ""),
                exprs=exprs,
                name=name,
                t=t,
                ty=ty,
                vec_t=vec_t,
                vl=vl,
            )


clamp = ""

for ty in elems:
    for vl in ns:
        clamp += render(
            """
            typename sycl::vec<${ty}, ${vl}>::vector_t
            __charm_sycl_clamp_v${vl}${t}(
                typename sycl::vec<${ty}, ${vl}>::vector_t x,
                typename sycl::vec<${ty}, ${vl}>::vector_t minval,
                typename sycl::vec<${ty}, ${vl}>::vector_t maxval
            ) {
                return __charm_sycl_${f}min_v${vl}${t}(
                    __charm_sycl_${f}max_v${vl}${t}(minval, x), maxval);
            }
            """,
            f=("f" if ty in floats else ""),
            vl=vl,
            t=sigs[ty],
            ty=ty,
        )

print(
    render(
        """
        #pragma once
        #include <charm/sycl.hpp>
        #include <cmath>
        #include <utility>

        CHARM_SYCL_BEGIN_NAMESPACE

        #ifndef __SYCL_DEVICE_ONLY__
        namespace runtime {
        $host
        }
        #endif

        namespace runtime {
        #include <charm/sycl/math_impl2.ipp>
        $element_wise
        $clamp
        }

        CHARM_SYCL_END_NAMESPACE
        """,
        host=host,
        element_wise=element_wise,
        clamp=clamp,
    )
)
