#!/usr/bin/env python3
"""Compare the standalone shim and repository runners on runtime failures."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import select
import shutil
import signal
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
TOOLS = ROOT / "tools/mips"


def result(command, env=None, stdin="", cwd=None):
    run = subprocess.run(command, input=stdin, capture_output=True, text=True,
                         timeout=30, env=env, cwd=cwd)
    return run.returncode, run.stdout, run.stderr


def check_interrupt(command, env, sig):
    with subprocess.Popen([*command, "wait"], env=env, text=True, stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE) as process:
        try:
            readable, _, _ = select.select([process.stdout], [], [], 10)
            assert readable and process.stdout.readline() == "ready\n", command
            process.send_signal(sig)
            # Keep stdin open until the signal arrives so the guest stays blocked.
            assert process.wait(timeout=10) == 128 + sig, (command, sig, process.returncode)
            assert process.stdout.read() == "" and process.stderr.read() == "", (command, sig)
        finally:
            process.stdin.close()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


# Builds runtime probes and compares launcher behavior for errors, signals,
# argument passing, standard streams, and exit status.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, default=ROOT / "build/baka")
    compiler = parser.parse_args().compiler.resolve()
    source = HERE / "runtime_errors.cpp"

    with tempfile.TemporaryDirectory(prefix="baka-runtime-") as directory:
        work = Path(directory)
        shim = work / 'standalone "shim"'
        assert result(["make", "-s", "-C", ROOT, "shim", f"out={shim}"]) == (0, "", "")
        binary = work / 'runtime, probe: "mips"'
        raw = work / "runtime unoptimized"
        assert result([compiler, "-o", binary, source]) == (0, "", "")
        assert result([compiler, "--no-optimize", "-o", raw, source]) == (0, "", "")

        def launchers(target):
            return (([shim], {**os.environ, "BAKA_BINARY": str(target)}),
                    ([TOOLS / "run-binary", target], None))

        def compare(target, args, stdin=""):
            outcomes = [result([*command, *args], env=env, stdin=stdin)
                        for command, env in launchers(target)]
            assert outcomes[0] == outcomes[1], (args, outcomes)
            return outcomes[0]

        # A guest exit code may coincide with an exit code conventionally used
        # by Docker or a shell. Only a fault should produce a fault diagnostic.
        statuses = [0, 1, 42, 125, 126, 127, 139, -1, 256]

        def check_exit(status):
            assert compare(binary, ["return", str(status)]) == (status & 255, "", "")

        def check_fault(target, args, sig, message):
            rc, out, err = compare(target, args)
            assert rc == 128 + sig and not out and message in err, (args, rc, out, err)

        with ThreadPoolExecutor(max_workers=4) as pool:
            list(pool.map(check_exit, statuses))
            faults = [
                (["load"], signal.SIGSEGV, "Segmentation fault"),
                (["store"], signal.SIGSEGV, "Segmentation fault"),
                (["unaligned"], signal.SIGBUS, "Bus error"),
                (["abort"], signal.SIGABRT, "Aborted"),
            ]
            list(pool.map(lambda entry: check_fault(*entry),
                          [(target, *fault) for target in (binary, raw) for fault in faults]))

            # Linux MIPS uses these signal numbers for the explicitly raised
            # signals. Compare with host signal names at the process boundary.
            raised = [(4, signal.SIGILL), (5, signal.SIGTRAP), (8, signal.SIGFPE)]
            list(pool.map(lambda entry: check_fault(binary, ["kill", str(entry[0])],
                                                     entry[1], "uncaught target signal"), raised))

        assert compare(binary, ["file"]) == (3, "", "open failed\n")
        assert compare(binary, ["io"], "x") == (0, "stdout\nx", "stderr\n")
        assert compare(binary, ["io"]) == (5, "stdout\n", "stderr\n")
        division = compare(binary, ["divide", "0"])
        assert division == compare(raw, ["divide", "0"])
        print(f"Guest integer division by zero returned status {division[0]}", flush=True)

        for command, env in launchers(binary):
            for sig in (signal.SIGINT, signal.SIGTERM):
                check_interrupt(command, env, sig)

        rc, out, err = compare(work / "missing", [])
        assert rc == 2 and not out and err, (rc, out, err)

        invalid = work / "invalid executable"
        invalid.write_text("not an ELF\n")
        truncated = work / "truncated ELF"
        wrong_arch = work / "wrong architecture"
        executable = bytearray(binary.read_bytes())
        truncated.write_bytes(executable[:20])
        executable[18] = 62  # ELF machine number for x86-64.
        wrong_arch.write_bytes(executable)
        obj = work / "not linked.o"
        assert result([compiler, "--assemble", "-o", obj, source]) == (0, "", "")
        for target in (invalid, truncated, wrong_arch, obj):
            rc, out, err = compare(target, [])
            assert (rc, out, err) == (1, "", f"baka runner: {target}: expected a little-endian "
                                      "MIPS64 ELF executable\n"), (target, rc, out, err)

        # The assembly helper uses the same launcher after linking.
        assembly = work / "runtime.s"
        rc, text, err = result([compiler, "--codegen", source])
        assert rc == 0 and not err
        assembly.write_text(text)
        assert result([TOOLS / "run", assembly, "io"], stdin="x") == (0, "stdout\nx", "stderr\n")
        assert result([TOOLS / "run", assembly, "store"]) == compare(binary, ["store"])
        for sig in (signal.SIGINT, signal.SIGTERM):
            check_interrupt([TOOLS / "run", assembly], None, sig)
        harness = work / "harness.c"
        harness.write_text('int main(void) { return 17; }\n')
        assert result([TOOLS / "run", "-c", harness, assembly]) == (17, "", "")
        # Use the CTest compiler to exercise the source recipe without rebuilding.
        (work / "build").mkdir()
        (work / "build/baka").symlink_to(compiler)
        (work / "tools").symlink_to(ROOT / "tools", target_is_directory=True)
        make_source = work / 'runtime "source".cpp'
        shutil.copyfile(source, make_source)
        make = ["make", "-s", "-f", ROOT / "Makefile", "-o", "build"]
        make_env = {**os.environ, "LC_ALL": "C"}
        for recipe, target in (("run-mips-binary", binary), ("run", make_source)):
            rc, out, err = result([*make, recipe, f"file={target}", "args=store"],
                                  env=make_env, cwd=work)
            assert rc == 2 and not out and "Segmentation fault" in err and "Error 139" in err, (rc, out, err)
        assembly.write_text(".not_a_valid_mips_directive\n")
        rc, out, err = result([TOOLS / "run", assembly])
        assert rc == 125 and not out and err, (rc, out, err)

    print("Runtime status, faults, signals, diagnostics and standard streams passed")


if __name__ == "__main__":
    main()
