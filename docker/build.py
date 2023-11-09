#!/usr/bin/env python3

import subprocess
import os
import tarfile
import sys
import io
import time


def add_file(tar, name, data):
    if type(data) == str:
        data = data.encode("UTF-8")
    data = io.BytesIO(data)
    info = tarfile.TarInfo(name)
    info.size = data.getbuffer().nbytes
    info.mtime = time.time()
    tar.addfile(info, data)


def make_context(tar, dir, /, chdir=True, prefix=""):
    def git_ls_files(dir):
        output = subprocess.check_output(["git", "ls-files"], cwd=dir).decode("UTF-8")
        return [os.path.join(dir, path) for path in output.splitlines()]

    def is_submodule_dir(dir):
        git = os.path.join(dir, ".git")
        if os.path.isfile(git):
            with open(git) as f:
                if "gitdir:" in f.read():
                    return True
        return False

    def add(tar, dir):
        n_file, n_mod = 0, 0
        for file in git_ls_files(dir):
            if os.path.isdir(file):
                if is_submodule_dir(file):
                    nf, nm = add(tar, file)
                    n_file += nf
                    n_mod += nm + 1
            elif os.path.isfile(file):
                tar.add(file, arcname=os.path.join(prefix, file), recursive=False)
                n_file += 1
        return (n_file, n_mod)

    oldcwd = os.getcwd() if chdir else None
    try:
        if chdir:
            os.chdir(dir)
            dir = "."
        return add(tar, dir)
    finally:
        os.chdir(oldcwd)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--spack-install-flags", type=str)
    parser.add_argument("--target", type=str)
    parser.add_argument("--iris-dir", type=str, required=True)
    parser.add_argument("tag")
    opts = parser.parse_args()

    tag = opts.tag
    source_dir = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
    context = io.BytesIO()
    commit = (
        subprocess.check_output(["git", "rev-parse", "HEAD"]).strip().decode("UTF-8")
    )

    with tarfile.open(mode="w|gz", fileobj=context) as tar:
        make_context(tar, source_dir)
        make_context(tar, opts.iris_dir, prefix="iris")
        add_file(tar, "Dockerfile", open(f"Dockerfile.{tag}").read())

    args = []
    if opts.target:
        args += ["--target", opts.target]
    if opts.spack_install_flags:
        args += ["--build-arg", f"SPACK_INSTALL_FLAGS={opts.spack_install_flags}"]

    subprocess.run(
        [
            "docker",
            "buildx",
            "build",
            "-t",
            f"charm-sycl:{tag}",
            "--build-arg",
            f"GIT_COMMIT={commit}",
        ]
        + args
        + ["-"],
        input=context.getvalue(),
        check=True,
    )
