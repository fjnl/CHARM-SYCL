from __future__ import print_function
from vec_gen import *
import itertools

print(
    """
#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
"""
)

print("namespace runtime {")
print()

for ty, n in itertools.product(elems, ns):
    vt = "ext_vector_type({n})".format(n=n)
    print(
        "typedef {ty} __attribute__(({vt})) v{n}{s}_t;".format(
            ty=ty, n=n, s=sigs[ty], vt=vt
        )
    )
print()

for ty, n in itertools.product(elems, ns):
    attrs = dict(ty=ty, n=n, s=sigs[ty])
    print("CHARM_SYCL_HOST_INLINE {ty} ".format(**attrs))
    print("__charm_sycl_vec_ix_v{n}{s}(v{n}{s}_t vec, int i)".format(**attrs))
    print("#ifdef __SYCL_DEVICE_ONLY__")
    print(";")
    print("#else")
    print("{ return vec[i]; }")
    print("#endif")
    print()

    print("CHARM_SYCL_HOST_INLINE void ")
    print("__charm_sycl_vec_aS_v{n}{s}(v{n}{s}_t* vec, int i, {ty} v)".format(**attrs))
    print("#ifdef __SYCL_DEVICE_ONLY__")
    print(";")
    print("#else")
    print("{ (*vec)[i] = v; }")
    print("#endif")
    print()

    attrs["args"] = ",".join("{ty} v{i}".format(ty=ty, i=i) for i in range(n))
    attrs["vals"] = ",".join("v{i}".format(ty=ty, i=i) for i in range(n))

    print("CHARM_SYCL_HOST_INLINE v{n}{s}_t ".format(**attrs))
    print("__charm_sycl_vec_v{n}{s}({args})".format(**attrs))
    print("#ifdef __SYCL_DEVICE_ONLY__")
    print(";")
    print("#else")
    print("{{return (v{n}{s}_t){{ {vals} }}; }}".format(**attrs))
    print("#endif")
    print()

    print("CHARM_SYCL_HOST_INLINE v{n}{s}_t ".format(**attrs))
    print("__charm_sycl_vec_splat_v{n}{s}({ty} v)".format(**attrs))
    print("#ifdef __SYCL_DEVICE_ONLY__")
    print(";")
    print("#else")
    print("{{return (v{n}{s}_t)(v); }}".format(**attrs))
    print("#endif")
    print()

print("}")
print()
print("namespace detail {")
print()

print("template <class DataT, int NumElements, class Vec> auto __charm_sycl_vec_ix")
print("(Vec vec, int i) {")
call_builtin("[]", "vec", "i", return_=True)
print("}")
print()

print("template <class DataT, int NumElements, class Vec> void __charm_sycl_vec_aS")
print("(Vec* vec, int i, DataT v) {")
call_builtin("=", args=["vec", "i", "v"])
print("}")
print()

print("template <class DataT, int NumElements> auto __charm_sycl_vec_splat")
print("(DataT v) {")
call_builtin("splat", "v", return_=True)
print("}")
print()

print("}")
print()
print("namespace runtime {")
print()

for op in ["+", "-", "*", "/"] + ["%", "<<", ">>", "&", "|", "^"]:
    int_only = op not in ["+", "-", "*", "/"]

    for ty, vl in itertools.product(ints if int_only else elems, ns):
        sig = "v" + str(vl) + sigs[ty]
        attrs = dict(
            op=op,
            opsig=ops[op],
            sig=sig,
            vec="{sig}_t".format(vl=vl, sig=sig),
            scalar=ty,
            vl=vl,
        )

        show(
            """
            CHARM_SYCL_HOST_INLINE ${vec} __charm_sycl_vec_${opsig}_${sig}(
                ${vec} x, ${vec} y
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${vec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${sig}(&r, i,
                        __charm_sycl_vec_ix_${sig}(x, i) ${op}
                        __charm_sycl_vec_ix_${sig}(y, i));
                }
                return r;
            #else
                return x ${op} y;
            #endif
            }

            CHARM_SYCL_HOST_INLINE ${vec} __charm_sycl_vec_${opsig}_${sig}_sv(
                ${scalar} x, ${vec} y
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${vec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${sig}(&r, i,
                        x ${op} __charm_sycl_vec_ix_${sig}(y, i));
                }
                return r;
            #else
                return x ${op} y;
            #endif
            }

            CHARM_SYCL_HOST_INLINE ${vec} __charm_sycl_vec_${opsig}_${sig}_vs(
                ${vec} x, ${scalar} y
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${vec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${sig}(&r, i,
                        __charm_sycl_vec_ix_${sig}(x, i) ${op} y);
                }
                return r;
            #else
                return x ${op} y;
            #endif
            }
            """,
            **attrs
        )

for op in ["+=", "-=", "*=", "/="] + ["%=", "<<=", ">>=", "&=", "|=", "^="]:
    int_only = op not in ["+=", "-=", "*=", "/="]

    for ty, vl in itertools.product(ints if int_only else elems, ns):
        sig = "v" + str(vl) + sigs[ty]
        attrs = dict(
            op=op,
            op0=op[0],
            opsig=ops[op],
            sig=sig,
            vec="{sig}_t".format(vl=vl, sig=sig),
            scalar=ty,
            vl=vl,
        )

        show(
            """
            CHARM_SYCL_HOST_INLINE void __charm_sycl_vec_${opsig}_${sig}(
                ${vec}& x, ${vec} y
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${sig}(&x, i,
                        __charm_sycl_vec_ix_${sig}(x, i) ${op0}
                        __charm_sycl_vec_ix_${sig}(y, i));
                }
            #else
                x ${op} y;
            #endif
            }

            CHARM_SYCL_HOST_INLINE void __charm_sycl_vec_${opsig}_${sig}_vs(
                ${vec}& x, ${scalar} y
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${sig}(&x, i,
                        __charm_sycl_vec_ix_${sig}(x, i) ${op0} y);
                }
            #else
                x ${op} y;
            #endif
            }
            """,
            **attrs
        )

for op in ["++", "--"]:
    for ty, vl in itertools.product(elems, ns):
        sig = "v" + str(vl) + sigs[ty]
        attrs = dict(
            op=op,
            op0=op[0],
            opsig=ops[op],
            op_sig=ops[op + "_"],
            sig=sig,
            vec="{sig}_t".format(vl=vl, sig=sig),
            scalar=ty,
            vl=vl,
        )
        show(
            """
            CHARM_SYCL_HOST_INLINE void __charm_sycl_vec_${opsig}_${sig}(
                ${vec}& x
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${sig}(&x, i,
                        __charm_sycl_vec_ix_${sig}(x, i) + 1);
                }
            #else
                x ${op0}= 1;
            #endif
            }

            CHARM_SYCL_HOST_INLINE ${vec} __charm_sycl_vec_${op_sig}_${sig}(
                ${vec} x
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${vec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${sig}(&r, i,
                        __charm_sycl_vec_ix_${sig}(x, i) + 1);
                }
                return r;
            #else
                return x ${op0} 1;
            #endif
            }
            """,
            **attrs
        )

for op in ["+", "-", "~"]:
    int_only = op == "~"

    for ty, vl in itertools.product(ints if int_only else elems, ns):
        sig = "v" + str(vl) + sigs[ty]
        attrs = dict(
            op=op,
            opsig=ops[op + "_"],
            sig=sig,
            vec="{sig}_t".format(vl=vl, sig=sig),
            scalar=ty,
            vl=vl,
        )
        show(
            """
            CHARM_SYCL_HOST_INLINE ${vec} __charm_sycl_vec_${opsig}_${sig}(
                ${vec} x
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${vec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${sig}(&r, i,
                        ${op} __charm_sycl_vec_ix_${sig}(x, i));
                }
                return r;
            #else
                return ${op}x;
            #endif
            }
            """,
            **attrs
        )

for op in ["&&", "||", "==", "!=", "<", ">", "<=", ">="]:
    for ty, vl in itertools.product(elems, ns):
        sig = "v" + str(vl) + sigs[ty]
        boolsig = "v" + str(vl) + sigs[bool_type[ty]]
        attrs = dict(
            op=op,
            opsig=ops[op],
            sig=sig,
            boolsig=boolsig,
            boolvec=boolsig + "_t",
            vec=sig + "_t",
            scalar=ty,
            bool=bool_type[ty],
            vl=vl,
        )

        show(
            """
            CHARM_SYCL_HOST_INLINE ${boolvec} __charm_sycl_vec_${opsig}_${sig}(
                ${vec} x, ${vec} y
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${boolvec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${boolsig}(&r, i,
                        __charm_sycl_vec_ix_${sig}(x, i) ${op}
                        __charm_sycl_vec_ix_${sig}(y, i) ? (${bool})-1 : 0);
                }
                return r;
            #else
                return x ${op} y;
            #endif
            }

            CHARM_SYCL_HOST_INLINE ${boolvec} __charm_sycl_vec_${opsig}_${sig}_sv(
                ${scalar} x, ${vec} y
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${boolvec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${boolsig}(&r, i,
                        x ${op} __charm_sycl_vec_ix_${sig}(y, i) ? (${bool})-1 : 0);
                }
                return r;
            #else
                return x ${op} y;
            #endif
            }

            CHARM_SYCL_HOST_INLINE ${boolvec} __charm_sycl_vec_${opsig}_${sig}_vs(
                ${vec} x, ${scalar} y
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${boolvec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${boolsig}(&r, i,
                        __charm_sycl_vec_ix_${sig}(x, i) ${op} y ? (${bool})-1 : 0);
                }
                return r;
            #else
                return x ${op} y;
            #endif
            }
            """,
            **attrs
        )

for op in ["!"]:
    for ty, vl in itertools.product(elems, ns):
        sig = "v" + str(vl) + sigs[ty]
        boolsig = "v" + str(vl) + sigs[bool_type[ty]]
        attrs = dict(
            op=op,
            opsig=ops[op],
            sig=sig,
            boolsig=boolsig,
            boolvec=boolsig + "_t",
            vec="{sig}_t".format(vl=vl, sig=sig),
            scalar=ty,
            bool=bool_type[ty],
            vl=vl,
        )

        show(
            """
            CHARM_SYCL_HOST_INLINE ${boolvec} __charm_sycl_vec_${opsig}_${sig}(
                ${vec} x
            ) {
            #ifdef __SYCL_DEVICE_ONLY__
                ${boolvec} r;
                for (int i = 0; i < ${vl}; i++) {
                    __charm_sycl_vec_aS_${boolsig}(&r, i,
                        __charm_sycl_vec_ix_${sig}(x, i) ? (${bool})-1 : 0);
                }
                return r;
            #else
                return ${op}x;
            #endif
            }
            """,
            **attrs
        )

print("}")
print()

aliases = {
    "char": "int8_t",
    "uchar": "uint8_t",
    "short": "int16_t",
    "ushort": "uint16_t",
    "int": "int32_t",
    "uint": "uint32_t",
    "long": "int64_t",
    "ulont": "uint64_t",
    "float": "float",
    "double": "double",
}

for k, v in pairs(aliases):
    for n in [2, 3, 4, 8, 16]:
        print("using {k}{n} = vec<{v}, {n}>;".format(k=k, n=n, v=v))
    print()

print("CHARM_SYCL_END_NAMESPACE")
