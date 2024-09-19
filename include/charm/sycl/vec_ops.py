from __future__ import print_function
from vec_gen import *

print("operator DataT() const requires (NumElements == 1) { return v_[0]; }")
print()

for op in ["+", "-", "*", "/"]:
    print("friend vec operator {op}(vec const& lhs, vec const& rhs)".format(op=op))
    print("{")
    call_builtin(op, args=["lhs.v_", "rhs.v_"], pre="return vec(", post=")")
    print("}")
    print()

    print("friend vec operator {op}(vec const& lhs, DataT const& rhs)".format(op=op))
    print("{")
    call_builtin(op, args=["lhs.v_", "rhs"], pre="return vec(", post=")", opt="_vs")
    print("}")
    print()

    print("friend vec operator {op}(DataT const& lhs, vec const& rhs)".format(op=op))
    print("{")
    call_builtin(op, args=["lhs", "rhs.v_"], pre="return vec(", post=")", opt="_sv")
    print("}")
    print()

    print("friend vec& operator {op}=(vec& lhs, vec const& rhs)".format(op=op))
    print("{")
    call_builtin(op + "=", args=["lhs.v_", "rhs.v_"])
    print("return lhs;")
    print("}")
    print()

    print("friend vec& operator {op}=(vec& lhs, DataT const& rhs)".format(op=op))
    print("{")
    call_builtin(op + "=", args=["lhs.v_", "rhs"])
    print("return lhs;")
    print("}")
    print()

for op in ["++", "--"]:
    print(
        "friend vec& operator {op}(vec& v) requires (!std::is_same_v<DataT, bool>)".format(
            op=op
        )
    )
    print("{")
    call_builtin(op, args=["v.v_"])
    print("return v;")
    print("}")
    print()

    print(
        "friend vec operator {op}(vec const& v, int) requires (!std::is_same_v<DataT, bool>)".format(
            op=op
        )
    )
    print("{")
    call_builtin(op + "_", args=["v.v_"], pre="return vec(", post=")")
    print("}")
    print()

for op in ["+", "-"]:
    print("friend vec operator {op}(vec const& v)".format(op=op))
    print("{")
    call_builtin(op + "_", args=["v.v_"], pre="return vec(", post=")")
    print("}")
    print()

for op in ["%", "<<", ">>", "&", "|", "^"]:
    print(
        "friend vec operator {op}(vec const& lhs, vec const& rhs) requires (!std::is_floating_point_v<DataT>)".format(
            op=op
        )
    )
    print("{")
    call_builtin(
        op, args=["lhs.v_", "rhs.v_"], pre="return vec(", post=")", elems_=ints
    )
    print("}")
    print()

    print(
        "friend vec operator {op}(vec const& lhs, DataT const& rhs) requires (!std::is_floating_point_v<DataT>)".format(
            op=op
        )
    )
    print("{")
    call_builtin(
        op, args=["lhs.v_", "rhs"], pre="return vec(", post=")", opt="_vs", elems_=ints
    )
    print("}")
    print()

    print(
        "friend vec operator {op}(DataT const& lhs, vec const& rhs) requires (!std::is_floating_point_v<DataT>)".format(
            op=op
        )
    )
    print("{")
    call_builtin(
        op, args=["lhs", "rhs.v_"], pre="return vec(", post=")", opt="_sv", elems_=ints
    )
    print("}")
    print()

    print(
        "friend vec& operator {op}=(vec& lhs, vec const& rhs) requires (!std::is_floating_point_v<DataT>)".format(
            op=op
        )
    )
    print("{")
    call_builtin(op + "=", args=["lhs.v_", "rhs.v_"], elems_=ints)
    print("return lhs;")
    print("}")
    print()

    print(
        "friend vec& operator {op}=(vec& lhs, DataT const& rhs) requires (!std::is_floating_point_v<DataT>)".format(
            op=op
        )
    )
    print("{")
    call_builtin(op + "=", args=["lhs.v_", "rhs"], elems_=ints)
    print("return lhs;")
    print("}")
    print()

for op in ["&&", "||", "==", "!=", "<", ">", "<=", ">="]:
    print(
        "friend bool_vec_t operator{op}(vec const& lhs, vec const& rhs)".format(op=op)
    )
    print("{")
    call_builtin(op, args=["lhs.v_", "rhs.v_"], pre="return bool_vec_t(", post=")")
    print("}")
    print()

    print(
        "friend bool_vec_t operator{op}(vec const& lhs, DataT const& rhs)".format(op=op)
    )
    print("{")
    call_builtin(op, args=["lhs.v_", "rhs"], pre="return bool_vec_t(", post=")")
    print("}")
    print()

    print(
        "friend bool_vec_t operator{op}(DataT const& lhs, vec const& rhs)".format(op=op)
    )
    print("{")
    call_builtin(op, args=["lhs", "rhs.v_"], pre="return bool_vec_t(", post=")")
    print("}")
    print()


print("friend vec operator ~(vec const& v) requires (!std::is_floating_point_v<DataT>)")
print("{")
call_builtin("~_", args=["v.v_"], pre="return vec(", post=")", elems_=ints)
print("}")
print()

print("friend bool_vec_t operator !(vec const& v)")
print("{")
call_builtin("!", args=["v.v_"], pre="return bool_vec_t(", post=")")
print("}")
print()
