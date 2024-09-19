import blas_config as cfg
import sys
import xml.etree.ElementTree as et

output = sys.argv[1]
configs = sys.argv[2:-1]
input = sys.argv[-1]

tree = et.parse(input)
root = tree.getroot()
cfg = cfg.read_blas_ini(configs)


def param(t, name=None, dcast=None):
    attrib = dict(type=t)
    if name is not None:
        attrib["name"] = name
    if dcast is not None:
        attrib["dcast"] = dcast
    return et.Element("param", attrib)


for func in cfg.functions:
    for dtype in func.prefixes:
        name = "rocblas_" + func.name(dtype).lower()
        realname = "rocblas_" + func.name(dtype).lower()
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

        el.append(param("handle_t", "ctx"))
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
        root.append(el)

tree.write(output)
