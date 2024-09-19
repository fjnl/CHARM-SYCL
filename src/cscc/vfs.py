import sys
import os


def prefix(path):
    return path.replace("/", "_").replace(".", "_").replace("-", "_")


if __name__ == "__main__":
    with open("vfs.ipp", "w") as f:
        for file in sys.argv[1:]:
            if file == "--clang":
                continue

            if file.startswith("/"):
                file = os.path.basename(file)
            var = f"{prefix(file)}_start"
            print(f'extern "C" char {var}[];', file=f)

        print("static void add_files(llvm::vfs::InMemoryFileSystem& fs) {", file=f)

        dir = "__runtime__"

        for file in sys.argv[1:]:
            if file == "--clang":
                dir = "__clang__"
                continue

            if file.startswith("/"):
                file = os.path.basename(file)
            var = f"{prefix(file)}_start"
            path = f"/{dir}/{file}"
            print(
                f'fs.addFile("{path}", 0, llvm::MemoryBuffer::getMemBuffer({var}));',
                file=f,
            )

        print("}", file=f)
