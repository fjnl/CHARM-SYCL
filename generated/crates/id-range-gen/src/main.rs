use anyhow::Result;
use clap::{Parser, ValueEnum};
use sailfish::TemplateOnce;
use std::{fs::File, io};

fn render_to(w: &mut impl io::Write, t: impl TemplateOnce) -> Result<()> {
    w.write_all(t.render_once()?.as_bytes())?;
    Ok(writeln!(w, "\n")?)
}

#[derive(Debug, PartialEq, Eq, Clone)]
struct Op {
    str: String,
}

static ALL_OPS: std::sync::OnceLock<Vec<Op>> = std::sync::OnceLock::new();

impl Op {
    fn all() -> &'static Vec<Op> {
        ALL_OPS.get_or_init(|| {
            [
                "==", "!=", "+", "-", "*", "/", "%", "<<", ">>", "&", "|", "^", "&&", "||", "<",
                ">", "<=", ">=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "|=", "^=",
            ]
            .iter()
            .map(|s| Op { str: s.to_string() })
            .collect()
        })
    }

    fn is_rel(&self) -> bool {
        ["==", "!="].into_iter().any(|x| x == self.str)
    }

    fn is_bin(&self) -> bool {
        [
            "+", "-", "*", "/", "%", "<<", ">>", "&", "|", "^", "&&", "||", "<", ">", "<=", ">=",
        ]
        .into_iter()
        .any(|x| x == self.str)
    }

    fn is_compound(&self) -> bool {
        ["+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "|=", "^="]
            .into_iter()
            .any(|x| x == self.str)
    }
}

#[derive(ValueEnum, Debug, PartialEq, Eq, Clone, Copy)]

enum Target {
    ID,
    Range,
}

fn a_or_b<'a>(d: u8, a: &'a str, b: &'a str) -> &'a str {
    if d == 0 {
        a
    } else {
        b
    }
}

#[derive(TemplateOnce, Clone)]
#[template(path = "id-range.stpl")]
#[allow(unused_variables)]
struct Config {
    _target: Target,
    dim: u8,
    decl: bool,
    ops: &'static Vec<Op>,
}

impl Config {
    fn name(&self) -> &'static str {
        match self._target {
            Target::ID => "id",
            Target::Range => "range",
        }
    }

    fn is_id(&self) -> bool {
        matches!(self._target, Target::ID)
    }
}

fn render(cfg: Args) -> Result<Vec<u8>> {
    let mut buffer = Vec::new();

    render_to(
        &mut buffer,
        Config {
            _target: cfg.target,
            dim: cfg.dim,
            decl: cfg.decl,
            ops: Op::all(),
        },
    )?;
    Ok(buffer)
}

#[derive(Debug, Clone, Parser)]
struct Args {
    #[arg(short, long)]
    output: Option<String>,
    #[arg(long)]
    dim: u8,
    #[arg(long)]
    target: Target,
    #[arg(long)]
    decl: bool,
}

fn main() -> Result<()> {
    let args = Args::parse();

    let buffer = render(args.clone())?;
    let mut result = io::Cursor::new(buffer);

    if let Some(output) = args.output {
        let mut f = File::create(output)?;
        io::copy(&mut result, &mut f)?;
    } else {
        io::copy(&mut result, &mut io::stdout())?;
    }

    Ok(())
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_op_type() {
        for op in Op::all() {
            assert!(
                (op.is_bin() && !op.is_compound() && !op.is_rel())
                    || (!op.is_bin() && op.is_compound() && !op.is_rel())
                    || (!op.is_bin() && !op.is_compound() && op.is_rel()),
                "op={} bin={} compound={} rel={}",
                op.str,
                op.is_bin(),
                op.is_compound(),
                op.is_rel()
            );
        }
    }

    #[test]
    fn config() {
        let cfg_id = Config {
            _target: Target::ID,
            dim: 1,
            decl: true,
            ops: Op::all(),
        };

        assert!(cfg_id.is_id());
        assert_eq!(cfg_id.name(), "id");

        let cfg_range = Config {
            _target: Target::Range,
            dim: 1,
            decl: true,
            ops: Op::all(),
        };

        assert!(!cfg_range.is_id());
        assert_eq!(cfg_range.name(), "range");
    }
}
