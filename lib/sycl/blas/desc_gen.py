import io
import sys
from template import Template
from blas_config import *

T = """
{% if inst %}
#include "descs.hpp"
{% else %}
#include <charm/sycl/runtime/blas.hpp>
#include <blas/common.hpp>
#include <format.hpp>
{% end %}

CHARM_SYCL_BEGIN_NAMESPACE
namespace blas {

{% if inst %}
    {% for func in functions %}
        {% for p in func.prefixes %}
runtime::func_desc desc_{{func.name(p)}}_inst = {"{{func.name(p)}}", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
void const* const desc_{{func.name(p)}} = &desc_{{func.name(p)}}_inst;
        {% end %}
    {% end %}
{% else %}
    {% for func in functions %}
        {% for p in func.prefixes %}
extern runtime::func_desc desc_{{func.name(p)}}_inst;
        {% end %}
    {% end %}
{% end %}

}
CHARM_SYCL_END_NAMESPACE
""".lstrip()

argv = sys.argv.copy()
inst_mode = False

if "--cpp" in argv:
    argv.remove("--cpp")
    inst_mode = True

cfg = Config()
for ini in argv[2:]:
    data = read_blas_ini(ini)
    cfg.functions += data.functions

Template(T).save(argv[1], {}, functions=cfg.functions, inst=inst_mode)
