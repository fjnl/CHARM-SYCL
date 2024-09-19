use itertools::Itertools;
use std::fmt;
use std::io::Write;
use std::rc::Rc;

#[macro_export]
macro_rules! ty {
    [$($t:expr),*] => {
        vec![$($t.clone()),*]
    };
    [$($t:expr,)*] => {
        vec![$($t.clone(),)*]
    };
}

#[derive(Debug, Clone)]
pub enum Type {
    Char,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    USize,
    Void,
    Record(Rc<RecordType>),
    Tagged(Rc<TaggedType>),
    NativeR(Rc<RecordType>),
    NativeT(Rc<TaggedType>),
    Pointer(Rc<Type>),
    Const(Rc<Type>),
}

impl Type {
    pub fn into_native(&self) -> Type {
        match self {
            Type::Tagged(inner) => Type::NativeT(inner.clone()),
            Type::Record(inner) => Type::NativeR(inner.clone()),
            Type::NativeR(..) => panic!(),
            Type::NativeT(..) => panic!(),
            Type::Pointer(inner) => Type::Pointer(Rc::new(inner.into_native())),
            Type::Const(inner) => Type::Const(Rc::new(inner.into_native())),
            _ => self.clone(),
        }
    }

    pub fn p(&self) -> Type {
        Type::Pointer(Rc::new(self.clone()))
    }

    pub fn c(&self) -> Type {
        Type::Const(Rc::new(self.clone()))
    }

    pub fn cp(&self) -> Type {
        self.c().p()
    }

    pub fn cstr() -> Type {
        Type::Char.c().p()
    }

    pub fn voidp() -> Type {
        Type::Void.p()
    }

    pub fn voidcp() -> Type {
        Type::Void.cp()
    }

    pub fn voidpp() -> Type {
        Type::Void.p().p()
    }
}

impl From<TaggedType> for Type {
    fn from(value: TaggedType) -> Self {
        Type::Tagged(Rc::new(value))
    }
}

impl From<RecordType> for Type {
    fn from(value: RecordType) -> Self {
        Type::Record(Rc::new(value))
    }
}

impl fmt::Display for Type {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Type::Char => write!(f, "char"),
            Type::Int8 => write!(f, "int8_t"),
            Type::Int16 => write!(f, "int16_t"),
            Type::Int32 => write!(f, "int32_t"),
            Type::Int64 => write!(f, "int64_t"),
            Type::UInt8 => write!(f, "uint8_t"),
            Type::UInt16 => write!(f, "uint16_t"),
            Type::UInt32 => write!(f, "uint32_t"),
            Type::UInt64 => write!(f, "uint64_t"),
            Type::USize => write!(f, "size_t"),
            Type::Void => write!(f, "void"),
            Type::Record(t) => write!(f, "{}", t),
            Type::Tagged(t) => write!(f, "{}", t),
            Type::NativeT(t) => write!(f, "typename {}::native", t),
            Type::NativeR(..) => write!(f, "void*"),
            Type::Pointer(inner) => write!(f, "{}*", inner),
            Type::Const(inner) => write!(f, "{} const", inner),
        }
    }
}

#[derive(Debug, Clone)]
pub struct RecordType {
    size_byte: usize,
    align_byte: usize,
}

impl RecordType {
    #[allow(clippy::new_ret_no_self)]
    pub fn new(size_byte: usize, align_byte: usize) -> Type {
        Type::Record(Rc::new(Self {
            size_byte,
            align_byte,
        }))
    }
}

impl fmt::Display for RecordType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "detail::record_type<{}, {}>",
            self.size_byte, self.align_byte
        )
    }
}

#[derive(Debug, Clone)]
pub struct TaggedType {
    name: String,
    base: Type,
    tag: String,
    iv: Option<String>,
}

impl fmt::Display for TaggedType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.name)
    }
}

trait BuilderBase {
    fn make_tagged_type(name: &str, base: Type, tag: &str, iv: Option<&str>) -> TaggedType {
        TaggedType {
            name: name.to_string(),
            base,
            tag: tag.to_string(),
            iv: iv.map(|s| s.to_string()),
        }
    }
}

#[allow(private_bounds)]
pub trait Builder: BuilderBase {
    fn define_tagged_type_impl(
        &mut self,
        name: &str,
        base: Type,
        tag: &str,
        iv: Option<&str>,
    ) -> Type {
        Type::Tagged(Rc::new(Self::make_tagged_type(name, base, tag, iv)))
    }

    fn define_constant(&mut self, name: &str, ty: Type, val: &str);

    fn define_function(&mut self, name: &str, return_type: Type, args: &[Type]);

    fn define_fields(&mut self, rec: Type, fields: &[(&str, Type, usize)]);

    fn define_tagged_type(&mut self, name: &str, base: Type, tag: &str) -> Type {
        self.define_tagged_type_impl(name, base, tag, None)
    }

    fn define_tagged_type_iv(&mut self, name: &str, base: Type, tag: &str, iv: &str) -> Type {
        self.define_tagged_type_impl(name, base, tag, Some(iv))
    }

    fn define_enum(&mut self, name: &str, tag: &str) -> Type {
        self.define_tagged_type(name, Type::Int32, tag)
    }

    fn define_opaque_ptr(&mut self, name: &str, tag: &str) -> Type {
        self.define_tagged_type(name, Type::voidp(), tag)
    }
}

pub struct HeaderBuilder {
    buffer: Vec<u8>,
}

impl BuilderBase for HeaderBuilder {}

impl Builder for HeaderBuilder {
    fn define_tagged_type_impl(
        &mut self,
        name: &str,
        base: Type,
        tag: &str,
        iv: Option<&str>,
    ) -> Type {
        let tt = Self::make_tagged_type(name, base, tag, iv);

        let iv_s = if let Some(ref iv) = tt.iv {
            format!(", detail::init_val({})", iv)
        } else {
            String::new()
        };

        writeln!(
            self.buffer,
            r#"
            using {} = detail::tagged_t<this_type, {}, detail::tag_name("{}"){}>;
            "#,
            tt.name, tt.base, tt.tag, iv_s,
        )
        .unwrap();

        tt.into()
    }

    fn define_constant(&mut self, name: &str, ty: Type, val: &str) {
        writeln!(
            self.buffer,
            r#"
            static constexpr auto {name} = {ty}({val});
            "#
        )
        .unwrap();
    }

    fn define_function(&mut self, name: &str, return_type: Type, args: &[Type]) {
        let native_args = args.iter().cloned().map(|x| x.into_native()).join(", ");
        let arguments = args
            .iter()
            .enumerate()
            .map(|(i, arg)| format!("{} param{}", arg, i))
            .join(", ");
        let params = args
            .iter()
            .enumerate()
            .map(|(i, _)| format!("detail::unwrap(param{})", i))
            .join(",");
        let return_native = return_type.clone().into_native();

        writeln!(
            self.buffer,
            r#"
            private:
            static void* {name}_ptr;

            public:
            static inline auto {name}({arguments}) {{
                using Fn = {return_native} (*)({native_args});
                return detail::wrap<{return_type}>(
                    reinterpret_cast<Fn>({name}_ptr)({params}));
            }}
            "#,
        )
        .unwrap()
    }

    fn define_fields(&mut self, rec: Type, fields: &[(&str, Type, usize)]) {
        for (name, ty, offset) in fields {
            write!(
                self.buffer,
                r#"
                static inline void set_{name}({rec}& x, {ty} val) {{
                    set_<{offset}>(x, val);
                }}

                static inline auto get_{name}({rec} const& x) {{
                    return get_<{offset}, {ty}>(x);
                }}
                "#
            )
            .unwrap();
        }
    }
}

impl HeaderBuilder {
    pub fn new(name: &str) -> Self {
        let mut b = Self { buffer: Vec::new() };
        b.begin(name);
        b
    }

    fn begin(&mut self, name: &str) -> &mut Self {
        writeln!(
            self.buffer,
            "
            #pragma once

            #include <cstdint>
            #include <cstdlib>
            #include <memory>
            #include <string>
            #include <error.hpp>
            #include <interfaces.hpp>

            CHARM_SYCL_BEGIN_NAMESPACE

            namespace runtime {{
            struct {name} {{
                using this_type = {name};

                static error::result<void> init();
                static void close();
                static void clear();
                static std::string version_str();

                private:
                struct impl;
                static std::unique_ptr<impl> pimpl_;

                template <size_t Offset, class R, class T>
                [[maybe_unused]]
                static inline void set_(R& record, T val) {{
                    *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(record.address()) + Offset) = val;
                }}

                template <size_t Offset, class T, class R>
                [[maybe_unused]]
                static inline auto get_(R const& record) {{
                    return *reinterpret_cast<T const*>(reinterpret_cast<std::byte const*>(record.address()) + Offset);
                }}
                public:
            "
        )
        .unwrap();
        self
    }

    pub fn finish(mut self) -> String {
        writeln!(
            &mut self.buffer,
            "
            }};

            }}

            CHARM_SYCL_END_NAMESPACE
            "
        )
        .unwrap();
        String::from_utf8(self.buffer).unwrap()
    }
}

pub struct VarsBuilder {
    name: String,
    buffer: Vec<u8>,
}

impl BuilderBase for VarsBuilder {}

impl Builder for VarsBuilder {
    fn define_constant(&mut self, _name: &str, _ty: Type, _val: &str) {}

    fn define_fields(&mut self, _rec: Type, _fields: &[(&str, Type, usize)]) {}

    fn define_function(&mut self, name: &str, _return_type: Type, _args: &[Type]) {
        writeln!(self.buffer, "void* {}::{}_ptr = nullptr;", self.name, name).unwrap();
    }
}

impl VarsBuilder {
    pub fn new(name: &str) -> Self {
        Self {
            name: name.to_string(),
            buffer: Vec::new(),
        }
    }

    pub fn finish(self) -> String {
        String::from_utf8(self.buffer).unwrap()
    }
}

pub struct ClearBuilder {
    name: String,
    buffer: Vec<u8>,
}

impl BuilderBase for ClearBuilder {}

impl Builder for ClearBuilder {
    fn define_constant(&mut self, _name: &str, _ty: Type, _val: &str) {}

    fn define_fields(&mut self, _rec: Type, _fields: &[(&str, Type, usize)]) {}

    fn define_function(&mut self, name: &str, _return_type: Type, _args: &[Type]) {
        writeln!(self.buffer, "{}_ptr = nullptr;", name).unwrap();
    }
}

impl ClearBuilder {
    pub fn new(name: &str) -> Self {
        let mut b = Self {
            name: name.to_string(),
            buffer: Vec::new(),
        };
        b.begin();
        b
    }

    pub fn finish(mut self) -> String {
        self.end();
        String::from_utf8(self.buffer).unwrap()
    }

    fn begin(&mut self) {
        writeln!(self.buffer, "void {}::clear() {{", self.name).unwrap();
    }

    fn end(&mut self) {
        writeln!(self.buffer, "pimpl_.reset();}}").unwrap();
    }
}

#[cfg(test)]
mod test {
    use super::HeaderBuilder;

    fn check_paren_balance(s: &str) -> isize {
        s.chars().fold(0_isize, |acc, ch| {
            if ['(', '{', '<', '['].contains(&ch) {
                acc + 1
            } else if [')', '}', '>', ']'].contains(&ch) {
                acc - 1
            } else {
                acc
            }
        })
    }

    #[test]
    fn builder() {
        let b = HeaderBuilder::new("name");
        let result = b.finish();

        assert_eq!(check_paren_balance(&result), 0);
        assert!(result.contains("struct name"));
        assert!(result.contains("using this_type = name;"));
        assert!(result.contains("static error::result<void> init();"));
        assert!(result.contains("static void close();"));
    }
}
