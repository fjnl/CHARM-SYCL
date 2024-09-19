from __future__ import print_function
from vec_gen import *


def template_params(type, var, n):
    return ",".join([type + str(i) + " " + var + str(i) for i in range(n)])


def params(type, var, n):
    return ",".join([type + " " + var + str(i) for i in range(n)])


def args(var, n):
    return ",".join([var + str(i) for i in range(n)])


def scalar(concept, types, name, n_args, dev_inline=False):
    return render(
        """
        namespace runtime {
            ${fwds}
        }
                    
        template <${template_params}> inline auto $name(${params}) {
            if constexpr (false) {}
            $body
            else {
                static_assert(not_supported<${template_types}>);
            }
        }
        """,
        concept=concept,
        name=name,
        template_params=params("detail::" + concept, "T", n_args),
        template_types=args("T", n_args),
        params=template_params("T", "x", n_args),
        fwds="\n".join(
            render(
                "${inline} $ty __charm_sycl_${name}_${sig}(${params});",
                ty=ty,
                name=name,
                sig=sigs[ty],
                params=params(ty, "x", n_args),
                inline=("inline" if dev_inline else "CHARM_SYCL_HOST_INLINE"),
            )
            for ty in types
        ),
        body="\n".join(
            render(
                """
                else if constexpr(detail::check_type_v<${ty}, ${ts}>) {
                    return runtime::__charm_sycl_${name}_${sig}(${args});
                }
                """,
                ty=ty,
                name=name,
                sig=sigs[ty],
                ts=args("T", n_args),
                args=args("x", n_args),
            )
            for ty in types
        ),
    )


def vector(
    concept,
    types,
    name,
    n_args,
    veclen=None,
    reduce=False,
    scalars=[],
    dev_inline=False,
):
    if not veclen:
        veclen = ns
    sigs_ = [render("v${n}${t}", n=n, t=sigs[ty]) for ty in types for n in veclen]
    types = [
        render("sycl::vec<${ty}, ${n}>", ty=ty, n=n) for ty in types for n in veclen
    ]
    if len(scalars) > 0:
        tp = params("detail::" + concept, "T", 1)
        ntp = 1
        ts = args("T", 1)
        ps = []
        lower_params = []
        for i in range(n_args):
            if i in scalars:
                ps.append("typename T0::value_type x{i}".format(i=i))
                lower_params.append("typename {{ty}}::element_type x{i}".format(i=i))
            else:
                ps.append("T{i} const& x{i}".format(i=i))
                lower_params.append("typename {{ty}}::vector_t x{i}".format(i=i))
        ps = ",".join(ps)
        lower_params = ",".join(lower_params)
        sig_ex = "_" + "".join("s" if i in scalars else "v" for i in range(n_args))
    else:
        tp = params("detail::" + concept, "T", n_args)
        ntp = n_args
        ts = args("T", n_args)
        ps = template_params("T", "const& x", n_args)
        lower_params = params("typename {ty}::vector_t", "x", n_args)
        sig_ex = ""

    return render(
        """
        namespace runtime {
            ${fwds}
        }
                    
        template <${template_params}> inline auto $name(${params})
            requires (detail::Compatible${ntp}<${template_types}>) {
            if constexpr (false) {}
            $body
            else {
                static_assert(not_supported<${template_types}>);
            }
        }
        """,
        concept=concept,
        name=name,
        n_args=n_args,
        ntp=ntp,
        template_params=tp,
        template_types=ts,
        params=ps,
        fwds="\n".join(
            render(
                "${inline} ${ret} __charm_sycl_${name}_${sig}(${params});",
                ret=(
                    "typename " + ty + "::element_type"
                    if reduce
                    else "typename " + ty + "::vector_t"
                ),
                name=name,
                sig=sig + sig_ex,
                params=lower_params.format(ty=ty),
                inline=("inline" if dev_inline else "CHARM_SYCL_HOST_INLINE"),
            )
            for ty, sig in zip(types, sigs_)
        ),
        body="\n".join(
            render(
                """
                else if constexpr (std::is_same_v<T0, ${ty}>) {
                    return ${ret_ty}(runtime::__charm_sycl_${name}_${sig}(${args}));
                }
                """,
                ty=ty,
                ret_ty=("" if reduce else ty),
                name=name,
                sig=sig + sig_ex,
                args=args("x", n_args),
            )
            for ty, sig in zip(types, sigs_)
        ),
    )


body = ""

for fn in math_funcs:
    name = fn.split(":")[0]
    dev_inline = False

    if name in ["clamp"]:
        dev_inline = True

    if fn.endswith(":gfgfgf"):
        body += scalar("GenFloat", floats, name, 3, dev_inline=dev_inline)
        body += vector("GenFloatVec", floats, name, 3, dev_inline=dev_inline)
    elif fn.endswith(":gigigi"):
        body += scalar("GenInt", ints, name, 3, dev_inline=dev_inline)
        body += vector("GenIntVec", ints, name, 3, dev_inline=dev_inline)
    elif fn.endswith(":fvss"):
        body += vector(
            "GenFloatVec", floats, name, 3, scalars=[1, 2], dev_inline=dev_inline
        )
    elif fn.endswith(":ivss"):
        body += vector(
            "GenIntVec", ints, name, 3, scalars=[1, 2], dev_inline=dev_inline
        )
    elif fn.endswith(":gfgf"):
        body += scalar("GenFloat", floats, name, 2, dev_inline=dev_inline)
        body += vector("GenFloatVec", floats, name, 2, dev_inline=dev_inline)
    elif fn.endswith(":gigi"):
        body += scalar("GenInt", ints, name, 2, dev_inline=dev_inline)
        body += vector("GenIntVec", ints, name, 2, dev_inline=dev_inline)
    elif fn.endswith(":geogeo"):
        body += scalar("GeoFloat", floats, name, 2, dev_inline=dev_inline)
        body += vector(
            "GeoFloatVec",
            floats,
            name,
            2,
            veclen=[2, 3, 4],
            reduce=True,
            dev_inline=dev_inline,
        )
    elif fn.endswith(":gf"):
        body += scalar("GenFloat", floats, name, 1, dev_inline=dev_inline)
        body += vector("GenFloatVec", floats, name, 1, dev_inline=dev_inline)
    elif fn.endswith(":gi"):
        body += scalar("GenInt", floats, name, 1, dev_inline=dev_inline)
        body += vector("GenIntVec", ints, name, 1, dev_inline=dev_inline)
    elif fn.endswith(":geo"):
        body += scalar("GeoFloat", floats, name, 1, dev_inline=dev_inline)
        body += vector(
            "GeoFloatVec",
            floats,
            name,
            1,
            veclen=[2, 3, 4],
            reduce=True,
            dev_inline=dev_inline,
        )
    else:
        assert False, fn

t = render(
    """
    #pragma once

    #include <cmath>
    #include <charm/sycl.hpp>

    CHARM_SYCL_BEGIN_NAMESPACE

    namespace detail {

    template <class T, class... Us>
    inline constexpr bool check_type_v = std::is_same_v<T, std::common_type_t<Us...>>;

    template <class T, class E>
    concept VecLike = is_vec_v<T> && std::is_same_v<typename T::element_type, E>;

    template <class T>
    concept GenInt = 
        std::is_same_v<T, char> ||
        std::is_same_v<T, unsigned char> ||
        std::is_same_v<T, short> ||
        std::is_same_v<T, unsigned short> ||
        std::is_same_v<T, int> ||
        std::is_same_v<T, unsigned int> ||
        std::is_same_v<T, long> ||
        std::is_same_v<T, unsigned long> ||
        std::is_same_v<T, long long> ||
        std::is_same_v<T, unsigned long long>;

    template <class T>
    concept GenIntVec =
        VecLike<T, char> ||
        VecLike<T, unsigned char> ||
        VecLike<T, short> ||
        VecLike<T, unsigned short> ||
        VecLike<T, int> ||
        VecLike<T, unsigned int> ||
        VecLike<T, long> ||
        VecLike<T, unsigned long> ||
        VecLike<T, long long> ||
        VecLike<T, unsigned long long>;

    template <class T>
    concept GenFloat = std::is_same_v<T, float> || std::is_same_v<T, double>;

    template <class T>
    concept GenFloatVec = VecLike<T, float> || VecLike<T, double>;

    template <class T>
    concept GeoFloat = 
        std::is_same_v<T, float> ||
        std::is_same_v<T, double>;

    template <class T>
    concept GeoFloatVec = 
        std::is_same_v<T, vec<float, 2>> ||
        std::is_same_v<T, vec<float, 3>> ||
        std::is_same_v<T, vec<float, 4>> ||
        std::is_same_v<T, vec<double, 2>> ||
        std::is_same_v<T, vec<double, 3>> ||
        std::is_same_v<T, vec<double, 4>>;
    
    template <class V1>
    concept Compatible1 = true;

    template <class V1, class V2>
    concept Compatible2 =
        std::is_same_v<typename V1::element_type, typename V2::element_type> &&
        (V1::size() == V2::size());

    template <class V1, class V2, class V3>
    concept Compatible3 = Compatible2<V1, V2> && Compatible2<V1, V3>;

    }

    $body

    CHARM_SYCL_END_NAMESPACE
    
    #include <charm/sycl/math_impl.hpp>
    """,
    body=body,
)

print(t)
