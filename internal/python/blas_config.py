import re
from configparser import ConfigParser


def comma_list(cfg, sct, opt, fallback=[]):
    val = cfg.get(sct, opt, fallback=None)
    if val is not None:
        return re.split(r"\s*,\s*", val)
    return fallback


class Parameter:
    def __init__(self, idx: int, arg: str):
        self.__idx = idx
        self.__arg = arg

    @property
    def index(self):
        return self.__idx

    @property
    def name(self):
        return self.__arg

    @property
    def is_trans(self):
        return self.__arg.startswith("TRANS")

    @property
    def is_uplo(self):
        return self.__arg == "UPLO"

    @property
    def is_diag(self):
        return self.__arg == "DIAG"

    @property
    def is_side(self):
        return self.__arg == "SIDE"

    @property
    def is_scalar(self):
        return self.__arg in ["ALPHA", "BETA"]

    @property
    def is_vector(self):
        return self.__arg in ["X", "Y", "Z"]

    @property
    def is_matrix(self):
        return self.__arg in ["A", "B", "C"]

    @property
    def is_dim(self):
        return self.__arg.startswith("LD")

    @property
    def is_size(self):
        return self.__arg in ["N", "M", "K"]


class DataType:
    def __init__(self, pre, **kwargs):
        mapping = dict(
            S="float", D="double", C="std::complex<float>", Z="std::complex<double>"
        )
        mapping.update(kwargs)
        self.scalar_type = mapping[pre]
        self.array_type = self.scalar_type + "*"
        self.vec_buffer = f"buffer<{self.scalar_type}, 1>"
        self.mtx_buffer = f"buffer<{self.scalar_type}, 2>"
        self.str = pre.lower()

    def is_complex(self):
        return self.str in ["c", "z"]

    def is_double_prec(self):
        return self.str in ["d", "z"]


class Function:
    def __init__(self, data, sct, *, param_cls=Parameter, dtype_cls=DataType):
        self.prefixes = [dtype_cls(p) for p in comma_list(data, sct, "prefixes")]
        self.args = [
            param_cls(i, a) for i, a in enumerate(comma_list(data, sct, "args"))
        ]
        self.common_name = sct.replace("x", "").lower()
        self.read = comma_list(data, sct, "read")
        self.readwrite = comma_list(data, sct, "readwrite")
        self.write = comma_list(data, sct, "write")
        self.cusolver = comma_list(data, sct, "cusolver", fallback=None)
        self.rocsolver = comma_list(data, sct, "rocsolver", fallback=None)

    def nparams(self):
        return len(self.args)

    def name(self, p):
        return p.str + self.common_name

    def desc(self, p):
        return f"desc_{self.name(p)}_inst"

    def ptr_arg(self, tpe, idx):
        return f"reinterpret_cast<{tpe}*>(args[{idx}])"

    def cast_arg(self, tpe, idx):
        return f"(*({self.ptr_arg(tpe, idx)}))"

    def params(self):
        result = []

        for arg in self.args:
            if arg.is_trans:
                result.append(f"trans_to_int({arg.name})")
            elif arg.is_uplo:
                result.append(f"uplo_to_int({arg.name})")
            elif arg.is_diag:
                result.append(f"diag_to_int({arg.name})")
            elif arg.is_side:
                result.append(f"side_to_int({arg.name})")
            elif arg.is_scalar:
                result.append(arg.name)
            elif arg.is_vector or arg.is_matrix:
                result.append(f"{arg.name} + {arg.name}_off")
            elif arg.is_size:
                result.append(arg.name)
            elif arg.is_dim:
                result.append(arg.name)
            else:
                result.append(arg.name)

        return result

    def vars(self, dtype):
        result = []
        index = 0
        for arg in self.args:
            expr = None
            if arg.is_trans:
                tpe = "blas::trans"
            elif arg.is_uplo:
                tpe = "blas::uplo"
            elif arg.is_diag:
                tpe = "blas::diag"
            elif arg.is_side:
                tpe = "blas::side"
            elif arg.is_scalar:
                tpe = dtype.scalar_type
            elif arg.is_vector or arg.is_matrix:
                tpe = dtype.array_type
            elif arg.is_size:
                tpe = "int64_t"
            elif arg.is_dim:
                tpe = "rts::accessor"
                expr = self.cast_arg(tpe, index) + ".size[2]"
            else:
                tpe = "int64_t"

            if expr is None:
                expr = self.cast_arg(tpe, index)
            result.append(f"auto {arg.name} = {expr}")
            index += 1

            if arg.is_vector or arg.is_matrix:
                expr = (
                    "rts::acc_linear_off(" + self.cast_arg("rts::accessor", index) + ")"
                )
                result.append(f"auto {arg.name}_off = {expr}")

        return result


class Config:
    def __init__(
        self, data=None, *, fn_cls=Function, param_cls=Parameter, dtype_cls=DataType
    ):
        self.functions = []
        if data is not None:
            for sct in data.sections():
                self.functions.append(
                    fn_cls(data, sct, param_cls=param_cls, dtype_cls=dtype_cls)
                )

    def select_cusolver(self):
        return [f for f in self.functions if f.cusolver]

    def select_rocsolver(self):
        return [f for f in self.functions if f.rocsolver]


def read_blas_ini(
    filename, *, fn_cls=Function, param_cls=Parameter, dtype_cls=DataType
):
    if isinstance(filename, list):
        result = Config()
        for file in filename:
            data = ConfigParser()
            data.read(file)
            temp = Config(data, fn_cls=fn_cls, param_cls=param_cls, dtype_cls=dtype_cls)
            result.functions += temp.functions
    else:
        data = ConfigParser()
        data.read(filename)
        result = Config(data, fn_cls=fn_cls, param_cls=param_cls, dtype_cls=dtype_cls)
    return result
