import io
import re
import sys


class Template:
    def __init__(self, input):
        if type(input) is str:
            self._input = io.StringIO(input)
        else:
            self._input = input
        self._indent = 0
        self._code = None

    def exec(self, context, **kwargs):
        if self._code is None:
            self._parse()
        out = io.StringIO()
        locals = dict()
        if context is not None:
            try:
                locals.update(**context._asdict())
            except AttributeError:
                locals.update(**context)
        locals.update(**kwargs)
        locals.update(_out=out, comma=lambda i: "," if i > 0 else " ")
        exec(self._code.getvalue(), None, locals)
        return out.getvalue()

    def render_to(self, output, context, **kwargs):
        print(self.exec(context, **kwargs), file=output)

    def run(self, context, **kwargs):
        self.render_to(sys.stdout, context, **kwargs)

    def save(self, filename, context, **kwargs):
        buffer = io.StringIO()
        self.render_to(buffer, context, **kwargs)
        data = buffer.getvalue()
        with open(filename, "w") as f:
            f.write(data)

    def _parse(self):
        self._code = io.StringIO()
        for line in self._input:
            spc = "  " * self._indent
            m = re.match(r"^\s*\{%(.*?)%\}\s*", line)
            kwd = m[1].strip() if m else ""
            if m and kwd == "end":
                assert self._indent > 0
                self._indent -= 1
            elif m and kwd.startswith("for ") or kwd.startswith("if "):
                print(spc + m[1].strip() + ":", file=self._code)
                print(spc + "  pass", file=self._code)
                self._indent += 1
            elif m and kwd == "else":
                print(spc[:-2] + "else:", file=self._code)
                print(spc + "pass", file=self._code)
            elif m:
                print(spc + m[1].strip(), file=self._code)
            else:
                last = 0
                for m in re.finditer(r"\{\{(.+?)\}\}", line):
                    if last < m.start():
                        print(
                            spc
                            + "print({}, end='', file=_out)".format(
                                repr(line[last : m.start()])
                            ),
                            file=self._code,
                        )
                    last = m.end()
                    print(
                        spc + f"print(str({m[1]}), end='', file=_out)", file=self._code
                    )

                if last < len(line):
                    print(
                        spc + "print({}, end='', file=_out)".format(repr(line[last:])),
                        file=self._code,
                    )
        assert self._indent == 0, str(self._indent)
