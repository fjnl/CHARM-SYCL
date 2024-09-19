use anyhow::Result;
use clap::Parser;
use ifgen::{ty, Builder, ClearBuilder, HeaderBuilder, RecordType, Type, VarsBuilder};
use std::{fs::File, io};

fn render_impl(b: &mut impl Builder) {
    let context_t = b.define_opaque_ptr("context_t", "CUcontext");
    let device_t = b.define_tagged_type_iv("device_t", Type::Int32, "CUdevice", "-2");
    let deviceptr_t = b.define_tagged_type("deviceptr_t", Type::UInt64, "CUdeviceptr");
    let function_t = b.define_opaque_ptr("function_t", "CUfunction");
    let module_t = b.define_opaque_ptr("module_t", "CUmodule");
    let stream_t = b.define_opaque_ptr("stream_t", "CUstream");
    let result_t = b.define_enum("result_t", "CUresult");
    let memcpy2d_t = b.define_tagged_type("memcpy2d_t", RecordType::new(128, 8), "CUDA_MEMCPY2D");
    let memcpy3d_t = b.define_tagged_type("memcpy3d_t", RecordType::new(200, 8), "CUDA_MEMCPY3D");
    let memorytype_t = b.define_enum("memorytype_t", "CUmemorytype");
    let datatype_t = b.define_enum("datatype_t", "cudaDataType");
    let dev_attr_t = b.define_enum("dev_attr_t", "CUdevice_attribute");

    b.define_constant("k_CUDA_SUCCESS", result_t.clone(), "0");
    b.define_constant("k_MEMORYTYPE_HOST", memorytype_t.clone(), "1");
    b.define_constant("k_MEMORYTYPE_DEVICE", memorytype_t.clone(), "2");
    b.define_constant("k_R_32F", datatype_t.clone(), "0");
    b.define_constant("k_C_32F", datatype_t.clone(), "4");
    b.define_constant("k_R_64F", datatype_t.clone(), "1");
    b.define_constant("k_C_64F", datatype_t.clone(), "5");
    b.define_constant(
        "k_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT",
        dev_attr_t.clone(),
        "16",
    );

    b.define_fields(
        memcpy2d_t.clone(),
        &[
            ("srcXInBytes", Type::UInt64, 0),
            ("srcY", Type::UInt64, 8),
            ("srcMemoryType", memorytype_t.clone(), 16),
            ("srcHost", Type::voidcp(), 24),
            ("srcDevice", deviceptr_t.clone(), 32),
            ("srcPitch", Type::UInt64, 48),
            ("dstXInBytes", Type::UInt64, 56),
            ("dstY", Type::UInt64, 64),
            ("dstMemoryType", memorytype_t.clone(), 72),
            ("dstHost", Type::voidp(), 80),
            ("dstDevice", deviceptr_t.clone(), 88),
            ("dstPitch", Type::UInt64, 104),
            ("WidthInBytes", Type::UInt64, 112),
            ("Height", Type::UInt64, 120),
        ],
    );

    b.define_fields(
        memcpy3d_t.clone(),
        &[
            ("srcXInBytes", Type::UInt64, 0),
            ("srcY", Type::UInt64, 8),
            ("srcZ", Type::UInt64, 16),
            ("srcLOD", Type::UInt64, 24),
            ("srcMemoryType", memorytype_t.clone(), 32),
            ("srcHost", Type::voidcp(), 40),
            ("srcDevice", deviceptr_t.clone(), 48),
            ("srcPitch", Type::UInt64, 72),
            ("srcHeight", Type::UInt64, 80),
            ("dstXInBytes", Type::UInt64, 88),
            ("dstY", Type::UInt64, 96),
            ("dstZ", Type::UInt64, 104),
            ("dstLOD", Type::UInt64, 112),
            ("dstMemoryType", memorytype_t.clone(), 120),
            ("dstHost", Type::voidp(), 128),
            ("dstDevice", deviceptr_t.clone(), 136),
            ("dstPitch", Type::UInt64, 160),
            ("dstHeight", Type::UInt64, 168),
            ("WidthInBytes", Type::UInt64, 176),
            ("Height", Type::UInt64, 184),
            ("Depth", Type::UInt64, 192),
        ],
    );

    let funcs = vec![
        ("cu_ctx_pop_current", result_t.clone(), ty![context_t.p()]),
        ("cu_ctx_set_current", result_t.clone(), ty![context_t]),
        (
            "cu_device_get",
            result_t.clone(),
            ty![device_t.p(), Type::Int32],
        ),
        (
            "cu_device_primary_ctx_release",
            result_t.clone(),
            ty![device_t],
        ),
        (
            "cu_device_primary_ctx_retain",
            result_t.clone(),
            ty![context_t.p(), device_t],
        ),
        (
            "cu_device_primary_ctx_set_flags",
            result_t.clone(),
            ty![device_t, Type::UInt32],
        ),
        (
            "cu_get_error_string",
            result_t.clone(),
            ty![result_t, Type::cstr().p()],
        ),
        ("cu_init", result_t.clone(), ty![Type::UInt32]),
        (
            "cu_launch_kernel",
            result_t.clone(),
            ty![
                function_t,
                Type::UInt32,
                Type::UInt32,
                Type::UInt32,
                Type::UInt32,
                Type::UInt32,
                Type::UInt32,
                Type::UInt32,
                stream_t,
                Type::voidpp(),
                Type::voidpp()
            ],
        ),
        (
            "cu_mem_alloc",
            result_t.clone(),
            ty![deviceptr_t.p(), Type::USize],
        ),
        (
            "cu_memcpy2d_async",
            result_t.clone(),
            ty![memcpy2d_t.cp(), stream_t],
        ),
        (
            "cu_memcpy3d_async",
            result_t.clone(),
            ty![memcpy3d_t.cp(), stream_t],
        ),
        (
            "cu_memcpy_dtod_async",
            result_t.clone(),
            ty![deviceptr_t, deviceptr_t, Type::USize, stream_t],
        ),
        (
            "cu_memcpy_dtoh_async",
            result_t.clone(),
            ty![Type::voidp(), deviceptr_t, Type::USize, stream_t],
        ),
        (
            "cu_memcpy_htod_async",
            result_t.clone(),
            ty![deviceptr_t, Type::voidcp(), Type::USize, stream_t],
        ),
        (
            "cu_memset_d8_async",
            result_t.clone(),
            ty![deviceptr_t, Type::UInt8, Type::USize, stream_t],
        ),
        ("cu_mem_free", result_t.clone(), ty![deviceptr_t]),
        (
            "cu_module_get_function",
            result_t.clone(),
            ty![function_t.p(), module_t, Type::cstr()],
        ),
        (
            "cu_module_load_fat_binary",
            result_t.clone(),
            ty![module_t.p(), Type::voidcp()],
        ),
        ("cu_module_unload", result_t.clone(), ty![module_t]),
        (
            "cu_stream_create",
            result_t.clone(),
            ty![stream_t.p(), Type::UInt32],
        ),
        ("cu_stream_destroy", result_t.clone(), ty![stream_t]),
        ("cu_stream_synchronize", result_t.clone(), ty![stream_t]),
        (
            "cu_mem_alloc_host",
            result_t.clone(),
            ty![Type::voidpp(), Type::UInt64],
        ),
        ("cu_mem_free_host", result_t.clone(), ty![Type::voidp()]),
        (
            "cuDeviceGetAttribute",
            result_t.clone(),
            ty![Type::Int32.p(), dev_attr_t, device_t],
        ),
        (
            "cu_mem_host_register",
            result_t.clone(),
            ty![Type::voidp(), Type::USize, Type::UInt32],
        ),
        (
            "cu_mem_host_unregister",
            result_t.clone(),
            ty![Type::voidp()],
        ),
    ];

    for (name, return_type, args) in funcs {
        b.define_function(name, return_type, &args);
    }
}

fn render(opt: &Args) -> String {
    let name = "cuda_interface";

    if opt.vars {
        let mut b = VarsBuilder::new(name);
        render_impl(&mut b);
        let mut c = ClearBuilder::new(name);
        render_impl(&mut c);
        format!("{}\n{}", b.finish(), c.finish())
    } else {
        let mut b = HeaderBuilder::new(name);
        render_impl(&mut b);
        b.finish()
    }
}

#[derive(Debug, Clone, Parser)]
struct Args {
    #[arg(short, long)]
    output: Option<String>,
    #[arg(long)]
    vars: bool,
}

fn main() -> Result<()> {
    let args = Args::parse();

    let buffer = render(&args);
    let mut result = io::Cursor::new(buffer);

    if let Some(output) = args.output {
        let mut f = File::create(output)?;
        io::copy(&mut result, &mut f)?;
    } else {
        io::copy(&mut result, &mut io::stdout())?;
    }

    Ok(())
}
