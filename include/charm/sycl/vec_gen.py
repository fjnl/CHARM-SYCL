import textwrap
import string

ns = [1, 2, 3, 4, 8, 16]
ints = [
    "char",
    "unsigned char",
    "short",
    "unsigned short",
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",
]
floats = [
    "float",
    "double",
]
elems = ints + floats
sizeof = {
    "char": 1,
    "unsigned char": 1,
    "short": 2,
    "unsigned short": 2,
    "int": 4,
    "unsigned int": 4,
    "long": 8,
    "unsigned long": 8,
    "long long": 8,
    "unsigned long long": 8,
    "float": 4,
    "double": 8,
}
bool_type = {
    "char": "char",
    "unsigned char": "char",
    "short": "short",
    "unsigned short": "short",
    "int": "int",
    "unsigned int": "int",
    "long": "long",
    "unsigned long": "long",
    "long long": "long",
    "unsigned long long": "long",
    "float": "int",
    "double": "long",
}
ops = {
    "+": "pl",
    "+_": "ps",
    "+=": "pL",
    "-": "ms",
    "-_": "ng",
    "-=": "mS",
    "*": "ml",
    "*=": "mL",
    "/": "dv",
    "/=": "dV",
    "<<": "ls",
    "<<=": "lS",
    ">>": "rs",
    ">>=": "rS",
    "++": "pp",
    "++_": "pp_",
    "--": "mm",
    "--_": "mm_",
    "%": "rm",
    "%=": "rM",
    "&": "an",
    "&=": "aN",
    "|": "or",
    "|=": "oR",
    "^": "eo",
    "^=": "eO",
    "&&": "aa",
    "||": "oo",
    "==": "eq",
    "!=": "ne",
    "<": "lt",
    ">": "gt",
    "<=": "le",
    ">=": "ge",
    "=": "aS",
    "[]": "ix",
    "!": "nt",
    "~_": "co",
}
sigs = {
    "char": "c",
    "unsigned char": "h",
    "short": "s",
    "unsigned short": "t",
    "int": "i",
    "unsigned int": "j",
    "long": "l",
    "unsigned long": "m",
    "long long": "x",
    "unsigned long long": "y",
    "float": "f",
    "double": "d",
}
bool_ops = [ops[name] for name in ["&&", "||", "==", "!=", "<", ">", "<=", ">=", "!"]]

math_funcs = [
    "acos:gf",
    "acosh:gf",
    "asin:gf",
    "asinh:gf",
    "atan:gf",
    # "atan2:gf",
    "atanh:gf",
    "cbrt:gf",
    "ceil:gf",
    "clamp:gfgfgf",
    "clamp:fvss",
    "clamp:gigigi",
    "clamp:ivss",
    "copysign:gfgf",
    "cos:gf",
    "cosh:gf",
    "distance:geogeo",
    "erf:gf",
    "erfc:gf",
    "exp:gf",
    # "exp10:gf",
    "exp2:gf",
    "expm1:gf",
    "fabs:gf",
    "fdim:gfgf",
    "floor:gf",
    "fmax:gfgf",
    "fmin:gfgf",
    "fmod:gfgf",
    "hypot:gfgf",
    "length:geo",
    "lgamma:gf",
    "log10:gf",
    "log1p:gf",
    "log2:gf",
    "logb:gf",
    "max:gigi",
    "min:gigi",
    "nextafter:gfgf",
    "pow:gfgf",
    "powr:gfgf",
    "remainder:gfgf",
    "rint:gf",
    "round:gf",
    "sin:gf",
    "sinh:gf",
    "sqrt:gf",
    "tan:gf",
    "tanh:gf",
    "tgamma:gf",
    "trunc:gf",
]


def render(template, **kwargs):
    return string.Template(textwrap.dedent(template.strip())).substitute(**kwargs)


def show(template, **kwargs):
    print(render(template, **kwargs))


def call_builtin(
    operator,
    lhs=None,
    rhs=None,
    opt=None,
    return_=False,
    ns_=None,
    args=None,
    pre="",
    post="",
    elems_=None,
):
    if opt is None:
        opt = ""
    if return_:
        pre = "return " + pre
    if ns_ is None:
        ns_ = ns
    if elems_ is None:
        elems_ = elems

    for i, ty in enumerate(elems_):
        if i == 0:
            if_ = "if constexpr"
            cond = "(std::is_same_v<DataT, {ty}>)".format(ty=ty)
        elif i == len(elems_) - 1:
            if_ = "else"
            cond = ""
        else:
            if_ = "else if constexpr"
            cond = "(std::is_same_v<DataT, {ty}>)".format(ty=ty)
        print("{if_}{cond} {{".format(if_=if_, cond=cond))

        for j, n in enumerate(ns_):
            if len(ns_) == 1:
                if_ = ""
                cond = ""
            elif j == 0:
                if_ = "if constexpr"
                cond = "(NumElements == {n})".format(n=n)
            elif j == len(ns_) - 1:
                if_ = "else"
                cond = ""
            else:
                if_ = "else if constexpr"
                cond = "(NumElements == {n})".format(n=n)
            if operator in ops:
                op = ops[operator]
            else:
                op = operator
            sig = sigs[ty]
            print("{if_}{cond} {{".format(if_=if_, cond=cond))

            if args:
                print(
                    "{pre}runtime::__charm_sycl_vec_{op}_v{n}{sig}{opt}({args}){post};".format(
                        op=op,
                        n=n,
                        sig=sig,
                        opt=opt,
                        pre=pre,
                        args=", ".join(args),
                        post=post,
                    )
                )
            elif rhs:
                print(
                    "{pre}runtime::__charm_sycl_vec_{op}_v{n}{sig}{opt}({lhs}, {rhs}){post};".format(
                        op=op,
                        n=n,
                        sig=sig,
                        lhs=lhs,
                        rhs=rhs,
                        opt=opt,
                        pre=pre,
                        post=post,
                    )
                )
            else:
                print(
                    "{pre}runtime::__charm_sycl_vec_{op}_v{n}{sig}{opt}({lhs}){post};".format(
                        op=op, n=n, sig=sig, lhs=lhs, opt=opt, pre=pre, post=post
                    )
                )
            print("}")

        print("}")


def values(dict):
    return [dict[k] for k in sorted(dict.keys())]


def pairs(dict):
    return [(k, dict[k]) for k in sorted(dict.keys())]
