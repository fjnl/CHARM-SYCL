from __future__ import print_function
import itertools

for i, v in enumerate("xyzw"):
    print(
        "auto {v}() const requires ({i} < NumElements && NumElements <= 4) {{ return swizzle<{i}>(); }}".format(
            v=v, i=i
        )
    )
    print()

for i, v in enumerate("rgba"):
    print(
        "auto {v}() const requires ({i} < NumElements && NumElements <= 4) {{ return swizzle<{i}>(); }}".format(
            v=v, i=i
        )
    )
    print()

for i in range(16):
    print(
        "auto s{i}() const requires ({i} < NumElements && NumElements <= 4) {{ return swizzle<{i}>(); }}".format(
            i=i
        )
    )
    print()

for n in range(2, 5):
    for indexes in itertools.permutations(range(n), n):
        method = "".join(["xyzw"[i] for i in indexes])
        idx_list = ", ".join([str(i) for i in indexes])
        print(
            "auto {method}() const requires ({n} <= NumElements && NumElements <= 4) {{ return swizzle<{idx_list}>(); }}".format(
                method=method, n=n, idx_list=idx_list
            )
        )
        print()

for n in range(2, 5):
    for indexes in itertools.permutations(range(n), n):
        method = "".join(["rgba"[i] for i in indexes])
        idx_list = ", ".join([str(i) for i in indexes])
        print(
            "auto {method}() const requires ({n} <= NumElements && NumElements <= 4) {{ return swizzle<{idx_list}>(); }}".format(
                method=method, n=n, idx_list=idx_list
            )
        )
        print()

for n in [2, 3, 4, 8, 16]:
    if n == 3:
        lo = "0, 1"
        hi = "2, -1"
        even = "0, 2"
        odd = "1, -1"
    else:
        lo = ", ".join([str(i) for i in range(n // 2)])
        hi = ", ".join([str(n // 2 + i) for i in range(n // 2)])
        even = ", ".join([str(i) for i in range(n) if i % 2 == 0])
        odd = ", ".join([str(i) for i in range(n) if i % 2 == 1])

    print(
        "auto lo() const requires (NumElements == {n}) {{ return swizzle<{lo}>(); }}".format(
            n=n, lo=lo
        )
    )
    print()

    print(
        "auto hi() const requires (NumElements == {n}) {{ return swizzle<{hi}>(); }}".format(
            n=n, hi=hi
        )
    )
    print()

    print(
        "auto even() const requires (NumElements == {n}) {{ return swizzle<{even}>(); }}".format(
            n=n, even=even
        )
    )
    print()

    print(
        "auto odd() const requires (NumElements == {n}) {{ return swizzle<{odd}>(); }}".format(
            n=n, odd=odd
        )
    )
    print()
