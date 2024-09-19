import blas_config as cfg
import sys
import xml.etree.ElementTree as et

output = sys.argv[1]
config = sys.argv[2]
input = sys.argv[3]

tree = et.parse(input)
root = tree.getroot()
cfg = cfg.read_blas_ini(config)


def param(t, name=None):
    attrib = dict(type=t)
    if name is not None:
        attrib["name"] = name
    return et.Element("param", attrib)


for func in cfg.functions:
    if not func.cusolver:
        continue

    p = func.prefixes[0]
    suffixes = [True, False] if "workspace" in func.cusolver else [False]
    for getbuffer in suffixes:
        pre = "x"
        name = func.cusolver[0].replace("cusolverDn", "cusolver_dn_").lower()
        if getbuffer:
            name += "_buffer_size"
        realname = func.cusolver[0]
        if getbuffer:
            realname += "_bufferSize"

        el = et.Element("function")
        el.attrib["return"] = "result_t"
        el.attrib["name"] = name
        el.attrib["realname"] = realname

        el.append(param("dnsolver_t", "handle"))
        el.append(param("params_t", "params"))
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
                el.append(param(p.scalar_type, arg.name))
            elif arg.is_vector or arg.is_matrix:
                el.append(param("datatype_t", arg.name + "Type"))
                el.append(param("void*", arg.name))
            else:
                el.append(param("int64_t", arg.name))
        if "prec" in func.cusolver:
            el.append(param("datatype_t", "computeType"))

        if getbuffer:
            el.append(param("size_t*", "d_buffer"))
            el.append(param("size_t*", "h_buffer"))
        else:
            if "workspace" in func.cusolver:
                el.append(param("void*", "d_buffer"))
                el.append(param("size_t", "d_bufferSize"))
                el.append(param("void*", "h_buffer"))
                el.append(param("size_t", "h_bufferSize"))
            el.append(param("int*", "info"))
        root.append(el)

# et.indent(tree)
tree.write(output)
