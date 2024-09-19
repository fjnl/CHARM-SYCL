import io
import re
import sys
import xml.etree.ElementTree as et


class Generator:
    def __init__(self, input, thin_initializer=False):
        self._root = et.parse(input).getroot()
        self._out = io.StringIO()
        self._typemap = {}
        self._thin_initializer = thin_initializer

    def run(self, output):
        self._start(self._root, thin_initializer=self._thin_initializer)
        for el in self._root:
            if el.tag == "type":
                self._type(el)
            elif el.tag == "function":
                self._func_wrapper(el, self._func_ptr(el))
            elif el.tag == "include":
                pass
            elif el.tag == "const":
                self._const(el)
            else:
                assert False, el.tag
        self._end()
        self.save(output)

    def save(self, output):
        data = self._out.getvalue()
        with open(output, "w") as f:
            f.write(data)

    def _start(self, node, *, thin_initializer=False, no_initializer=False):
        name = node.attrib["name"]
        print("#pragma once", file=self._out)
        print("#include <interfaces.hpp>", file=self._out)
        for inc in node.findall("include"):
            path = inc.text
            if int(inc.attrib.get("quote", "0")) != 0:
                path = '"' + path + '"'
            else:
                path = "<" + path + ">"
            print(f"#include {path}", file=self._out)
        print(file=self._out)
        print("CHARM_SYCL_BEGIN_NAMESPACE", file=self._out)
        print("namespace runtime {", file=self._out)
        print(file=self._out)
        print(f"struct {name} {{", file=self._out)
        print(f"using this_type = {name};", file=self._out)
        print(file=self._out)
        if not no_initializer:
            print(f"private:", file=self._out)
            print(f"static error::result<std::string> init_(void* h);", file=self._out)
            print(f"static void close_();", file=self._out)
            print(file=self._out)
        print(f"public:", file=self._out)
        print(f"static std::string version();", file=self._out)

        assert not (no_initializer and thin_initializer)

        if thin_initializer:
            print(
                f"static error::result<std::string> init(void* h) {{ return init_(h); }}",
                file=self._out,
            )
            print(f"static void close() {{ close_(); }}", file=self._out)
        elif no_initializer:
            print(
                f"static error::result<std::string> init() {{ return {{}}; }}",
                file=self._out,
            )
            print(f"static void close() {{}}", file=self._out)
        else:
            print(f"static error::result<std::string> init();", file=self._out)
            print(f"static void close();", file=self._out)
        print(file=self._out)

    def _end(self):
        print("};", file=self._out)
        print(file=self._out)
        print("CHARM_SYCL_END_NAMESPACE", file=self._out)
        print("}", file=self._out)

    def _const(self, node):
        type = node.attrib["type"]
        name = node.attrib["name"]
        value = node.attrib["value"]
        print(
            f"static constexpr {type} const {name} = {type}({value});", file=self._out
        )

    def _type(self, node):
        tag = node.attrib.get("tag", None)
        name = node.attrib["name"]
        type = node.attrib["type"]
        if tag:
            code = f'using {name} = detail::tagged_t<this_type, {type}, detail::tag_name("{tag}")>;'
        else:
            code = f"using {name} = {type};"
        print(code, file=self._out)
        print(file=self._out)
        self._typemap[name] = type

    def _lower(self, type):
        m = re.search(r".\b", type)
        if m:
            type, suffix = type[: m.end()], type[m.end() :]
        else:
            type, suffix = type, ""
        if type in self._typemap:
            type = f"{type}::native"
        return type + suffix

    def _func_ptr(self, node):
        return_t = self._lower(node.attrib["return"])
        name = node.attrib["name"] + "_ptr"
        params = []
        for param in node.findall("param"):
            params.append(self._lower(param.attrib["type"]))

        print("private:", file=self._out)
        print(f"static {return_t} (*{name})({', '.join(params)});", file=self._out)
        print(file=self._out)
        return name

    def _param_name(self, p, i):
        name = p.attrib.get("name", None)
        if name is None:
            return f"param{i}"
        return name

    def _param_cast(self, p, expr):
        return expr

    def _func_wrapper(self, node, fn_name):
        params = [
            f"{p.attrib['type']} {self._param_name(p, i)}"
            for i, p in enumerate(node.findall("param"))
        ]
        args = [
            self._param_cast(p, f"detail::unwrap({self._param_name(p, i)})")
            for i, p in enumerate(node.findall("param"))
        ]
        return_t = node.attrib["return"]
        name = node.attrib["name"]

        print("public:", file=self._out)
        print(f"static auto {name}({', '.join(params)}) {{", file=self._out)
        if return_t != "void":
            print(
                f"return detail::wrap<{return_t}>({fn_name}({', '.join(args)}));",
                file=self._out,
            )
        else:
            print(
                f"{fn_name}({', '.join(args)});",
                file=self._out,
            )
        print(f"}}", file=self._out)
        print(file=self._out)


class CppGenerator(Generator):
    def __init__(self, input, hpp):
        super().__init__(input)
        self._vars = []
        self._close = []
        self._hpp = hpp

    def _type(self, node):
        self._out, save = io.StringIO(), self._out
        super()._type(node)
        self._out = save

    def _const(self, node):
        pass

    def _start(self, node, *, thin_initializer=False, no_initializer=False):
        assert not thin_initializer
        assert not no_initializer

        name = node.attrib["name"]
        self._klass = name
        print(f'#include "{self._hpp}"', file=self._out)
        print(file=self._out)
        print("CHARM_SYCL_BEGIN_NAMESPACE", file=self._out)
        print("namespace runtime {", file=self._out)
        print(file=self._out)
        print(
            f"error::result<std::string> {self._klass}::init_(void* h) {{",
            file=self._out,
        )

    def _end(self):
        print("return {};", file=self._out)
        print("}", file=self._out)

        print(file=self._out)
        print(f"void {self._klass}::close_() {{", file=self._out)
        for stmt in self._close:
            print(stmt, file=self._out)
        print("}", file=self._out)

        print(file=self._out)
        for v in self._vars:
            print(v, file=self._out)

        print(file=self._out)
        print("}", file=self._out)
        print("CHARM_SYCL_END_NAMESPACE", file=self._out)

    def _func_wrapper(self, node, fn_name):
        realname = node.attrib["realname"]
        print(
            f'CHECK_ERROR(blas::load_func(h, {fn_name}, "{realname}"));', file=self._out
        )
        pass

    def _lower(self, type):
        t = super()._lower(type)
        if t.endswith("::native"):
            t = self._klass + "::" + t
        return t

    def _func_ptr(self, node):
        return_t = self._lower(node.attrib["return"])
        name = node.attrib["name"] + "_ptr"
        params = []
        for param in node.findall("param"):
            params.append(self._lower(param.attrib["type"]))
        params = ", ".join(params)

        self._vars.append(f"{return_t} (*{self._klass}::{name})({params});")
        self._close.append(f"{name} = nullptr;")

        return name


class DirectGenerator(Generator):
    def _start(self, node, *, thin_initializer=False, no_initializer=False):
        assert not thin_initializer
        return super()._start(node, no_initializer=True)

    def _func_ptr(self, node):
        return node.attrib["realname"]

    def _param_cast(self, p, expr):
        c = p.attrib.get("dcast", None)
        if c is None:
            return expr
        return f"reinterpret_cast<{c}>({expr})"


if __name__ == "__main__":
    args = sys.argv[1:]

    if args[0] == "--cpp":
        args.pop(0)
        hpp = args.pop(0)
        CppGenerator(args[1], hpp).run(args[0])
    elif args[0] == "--direct":
        args.pop(0)
        DirectGenerator(args[1]).run(args[0])
    else:
        thin_initializer = False
        if "--thin-init" in args:
            args.remove("--thin-init")
            thin_initializer = True
        Generator(args[1], thin_initializer=thin_initializer).run(args[0])
