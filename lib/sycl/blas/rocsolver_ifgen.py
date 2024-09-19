import blas_config as cfg
import sys
import xml.etree.ElementTree as et

output = sys.argv[1]
config = sys.argv[2]
input = sys.argv[3]

tree = et.parse(input)
root = tree.getroot()
cfg = cfg.read_blas_ini(config)


def param(t, name=None, dcast=None):
    attrib = dict(type=t)
    if name is not None:
        attrib["name"] = name
    if dcast is not None:
        attrib["dcast"] = dcast
    return et.Element("param", attrib)


for func in cfg.functions:
    if not func.rocsolver:
        continue

    for dtype in func.prefixes:
        name = "rocsolver_" + func.name(dtype)
        realname = name
        cmplx = None
        if dtype.is_complex():
            cmplx = (
                "rocblas_double_complex*"
                if dtype.is_double_prec()
                else "rocblas_float_complex*"
            )

        el = et.Element("function")
        el.attrib["return"] = "result_t"
        el.attrib["name"] = name
        el.attrib["realname"] = realname

        el.append(param("handle_t", "handle"))
        for arg in func.args:
            if arg.is_trans:
                el.append(param("trans_t", arg.name))
            elif arg.is_uplo:
                el.append(param("uplo_t", arg.name))
            elif arg.is_diag:
                el.append(param("diag_t", arg.name))
            elif arg.is_side:
                el.append(param("side_t", arg.name))
            elif arg.is_scalar:
                el.append(param(dtype.scalar_type + "*", arg.name, dcast=cmplx))
            elif arg.is_vector or arg.is_matrix:
                el.append(param(dtype.array_type, arg.name, dcast=cmplx))
            else:
                el.append(param("int32_t", arg.name))
        el.append(param("int*", "info"))
        root.append(el)

# et.indent(tree)
tree.write(output)
