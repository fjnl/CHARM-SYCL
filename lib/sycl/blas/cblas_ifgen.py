import blas_config as cfg
import sys
import xml.etree.ElementTree as et

args = sys.argv.copy()
if "--64" in args:
    args.remove("--64")
    suffix = "_64"
    fn_suffix = ""
    size_type = "int64_t"

    if "--suffix64" in args:
        args.remove("--suffix64")
        fn_suffix = "_64"
else:
    suffix = "_32"
    fn_suffix = ""
    size_type = "int32_t"

ns = args[1]
output = args[2]
configs = args[3:-1]
input = args[-1]

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


def subst(s):
    return s.replace("${suffix}", suffix)


def subst_attr(node, name):
    if name in node.attrib:
        node.attrib[name] = subst(node.attrib[name])


for func in cfg.functions:
    for dtype in func.prefixes:

        def char_or(t):
            return "char" if ns == "lapacke" else t

        basename = func.name(dtype).lower() + fn_suffix
        name = ns + "_" + basename
        if ns == "lapacke":
            realname = ns.upper() + "_" + basename
        else:
            realname = name
        cmplx = None
        scalar = dtype.scalar_type
        if dtype.is_complex() and ns != "lapacke":
            cmplx = "void*"
            scalar = scalar + "*"

        el = et.Element("function")
        el.attrib["return"] = "void"
        el.attrib["name"] = name
        el.attrib["realname"] = realname

        for arg in func.args:
            if arg.is_matrix:
                el.append(param("layout_t", "LAYOUT"))
                break

        for arg in func.args:
            if arg.is_trans:
                el.append(param(char_or("trans_t"), arg.name))
            elif arg.is_uplo:
                el.append(param(char_or("uplo_t"), arg.name))
            elif arg.is_diag:
                el.append(param(char_or("diag_t"), arg.name))
            elif arg.is_side:
                el.append(param(char_or("side_t"), arg.name))
            elif arg.is_scalar:
                el.append(param(scalar, arg.name, dcast=cmplx))
            elif arg.is_vector or arg.is_matrix:
                el.append(param(dtype.array_type, arg.name, dcast=cmplx))
            else:
                el.append(param(size_type, arg.name))
        root.append(el)

root = tree.getroot()
subst_attr(root, "name")
for node in root:
    if node.tag == "type":
        subst_attr(node, "type")
    elif node.tag == "include":
        node.text = subst(node.text)

tree.write(output)
