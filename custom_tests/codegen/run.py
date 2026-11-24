#!/usr/bin/env python3
"""Compile every fixture with --codegen, then assemble, link and run the batch in Docker/QEMU.

Fixtures come from ../ir/cases.json, ../ir_opt/cases.json and ./cases.json.
They use the same schema, one entry per source file:

  "NN_name.cpp": {
    "runs":  [[input, result], [input, result, "stdout"], ...],
    "stdin": "text piped to every run",          optional
    "global": ["name", value],                   optional
    "standalone": true,                          optional
    "failed_allocations": [[input, result], ...] optional
  }

  input   passed as argv[1]; a fixture reads it with `int n = atoi(argv[1]);`
  result  expected return value of main, read as a signed 32-bit integer
          (only the low 8 bits are observable for standalone runs)
  stdout  optional: exact text the program itself printed (the driver's marker lines excluded)
  global  source-level name of a global whose integer value (read after static destructors have
          run, from its `name.<id>` assembly symbol) must equal value; the id comes from `--ir`
  Empty `runs` only checks that the program assembles and links.

harness.c supplies the strong `main` except for standalone fixtures, which exercise the
generated entry point. Failed-allocation runs use a linker wrapper to fail the first
malloc after the harness starts the program. Harnessed programs are built as
  mips64el-linux-gnuabi64-gcc -march=mips64r2 -mabi=64 -static -no-pie -O0 x.s harness.c
and run as `qemu-mips64el ./x <input>` (all jobs in one `docker run`, built and run in parallel).
"""
import argparse
import difflib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
MARKER = "\x01"
C_TYPES = {"i8": "signed char", "u8": "unsigned char", "i16": "short", "u16": "unsigned short",
           "i32": "int", "u32": "unsigned int", "i64": "long long", "u64": "unsigned long long"}
MODES = {"opt": ["--codegen"], "noopt": ["--codegen", "--no-optimize"]}
BUILD = "mips64el-linux-gnuabi64-gcc -march=mips64r2 -mabi=64 -static -no-pie -O0"


def run(command, **kwargs):
    return subprocess.run([str(arg) for arg in command], capture_output=True, text=True, errors="replace", **kwargs)


def signed32(value):
    return ((value & 0xFFFFFFFF) ^ 0x80000000) - 0x80000000


def collect(names):
    """Return [(name, source path, case dict)] from the case manifests."""
    cases = []
    for directory in (HERE.parent / "ir", HERE.parent / "ir_opt", HERE):
        for name, case in json.loads((directory / "cases.json").read_text()).items():
            if not names or Path(name).stem in names or name in names:
                cases.append((name, directory / name, case))
    return cases


def execution_cases(case):
    yield from ((entry, False) for entry in case["runs"])
    yield from ((entry, True) for entry in case.get("failed_allocations", []))


def global_symbol(compiler, source, name):
    """Find `global <name>.<id>: <type>` in the IR; return (assembly symbol, C type)."""
    result = run([compiler, "--ir", source])
    match = re.search(rf"^global ({re.escape(name)}\.\d+): (\w+)", result.stdout, re.M)
    if not match or match.group(2) not in C_TYPES:
        raise RuntimeError(f"global '{name}' not found as an integer in --ir output")
    return match.group(1), C_TYPES[match.group(2)]


def prepare(args, tmp, cases):
    """Compile the .s files and write one job script per (case, mode). Return the job table."""
    jobs = []
    for name, source, case in cases:
        for mode in ("opt", "noopt"):
            if args.mode not in (mode, "both"):
                continue
            job = {"label": f"{name} [{mode}]", "id": f"{source.parent.name}.{source.stem}.{mode}",
                   "case": case, "error": None}
            jobs.append(job)
            result = run([args.compiler, *MODES[mode], source])
            if result.returncode:
                job["error"] = "baka failed:\n" + result.stderr + result.stdout[:2000]
                continue
            defs, text = "", result.stdout
            if "global" in case:
                try:
                    symbol, ctype = global_symbol(args.compiler, source, case["global"][0])
                except RuntimeError as error:
                    job["error"] = str(error)
                    continue
                defs = f"-DBAKA_GLOBAL_SYM='\"{symbol}\"' -DBAKA_GLOBAL_TYPE='{ctype}'"
                text += f"\n\t.globl {symbol}\n"  # make a possibly-local symbol visible to harness.c
            ident = job["id"]
            (tmp / f"{ident}.s").write_text(text)
            (tmp / f"{ident}.stdin").write_text(case.get("stdin", ""))
            # Parallel jobs may create files with the same names.
            (tmp / f"{ident}.cwd").mkdir()
            harness = "" if case.get("standalone") else "harness.c"
            if case.get("failed_allocations"):
                defs += " -DBAKA_WRAP_MALLOC -Wl,--wrap=malloc"
            lines = [f"{BUILD} {defs} {ident}.s {harness} -o {ident}.bin 2> {ident}.build",
                     f"echo $? > {ident}.brc", f"if [ \"$(cat {ident}.brc)\" = 0 ]; then", "true"]
            for index, (entry, fail_allocation) in enumerate(execution_cases(case)):
                env = "BAKA_FAIL_ALLOCATION=1 " if fail_allocation else ""
                lines.append(f"(cd {ident}.cwd && {env}timeout 30 qemu-mips64el ../{ident}.bin {entry[0]}) < {ident}.stdin "
                             f"> {ident}.{index}.out 2> {ident}.{index}.err; echo $? > {ident}.{index}.rc")
            lines.append("fi")
            (tmp / f"{ident}.job.sh").write_text("\n".join(lines) + "\n")
    return jobs


def execute(args, tmp):
    (tmp / "run_all.sh").write_text("cd /work\nls *.job.sh | xargs -P 8 -I{} sh /work/{}\nexit 0\n")
    command = ["docker", "run", "--rm", "--user", f"{os.getuid()}:{os.getgid()}",
               "-v", f"{tmp}:/work", args.image, "sh", "/work/run_all.sh"]
    return run(command, timeout=1800)


def read(tmp, name, default=""):
    path = tmp / name
    return path.read_text(errors="replace") if path.exists() else default


def show_diff(expected, actual, label):
    diff = difflib.unified_diff(expected.splitlines(True), actual.splitlines(True),
                                fromfile=f"expected {label}", tofile=f"actual {label}")
    return "".join(diff) or f"expected {expected!r}\n  actual {actual!r}"


def check(tmp, job):
    """Return a list of problem strings (empty means pass)."""
    if job["error"]:
        return [job["error"]]
    ident, case = job["id"], job["case"]
    if read(tmp, f"{ident}.brc", "missing").strip() != "0":
        return ["assemble/link failed:\n" + read(tmp, f"{ident}.build", "(job did not run)")]
    problems = []
    for index, (entry, fail_allocation) in enumerate(execution_cases(case)):
        out = read(tmp, f"{ident}.{index}.out")
        rc = read(tmp, f"{ident}.{index}.rc", "?").strip()
        where = f"input {entry[0]}"
        if fail_allocation:
            where += " (first allocation fails)"
        if case.get("standalone"):
            expected = str(entry[1] & 0xFF)
            if rc != expected:
                problems.append(f"{where}: exit code {rc}, expected {expected}\n"
                                + read(tmp, f"{ident}.{index}.err"))
            if len(entry) > 2 and out != entry[2]:
                problems.append(f"{where}: stdout differs\n" + show_diff(entry[2], out, "stdout"))
            continue
        text, marker, tail = out.partition(MARKER + "result=")
        if not marker:
            problems.append(f"{where}: no result line (exit code {rc}); stdout:\n{out}stderr:\n"
                            + read(tmp, f"{ident}.{index}.err"))
            continue
        values = dict(re.findall(r"(?:^|\x01)(result|global)=(-?\d+)", marker + tail))
        if int(values["result"]) != signed32(entry[1]):
            problems.append(f"{where}: result {values['result']}, expected {signed32(entry[1])}")
        if len(entry) > 2 and text != entry[2]:
            problems.append(f"{where}: stdout differs\n" + show_diff(entry[2], text, "stdout"))
        if "global" in case:
            expected = case["global"][1]
            actual = values.get("global")
            if actual is None or (int(actual) - expected) % 2 ** 64:
                problems.append(f"{where}: global {case['global'][0]} = {actual}, expected {expected}")
        if rc != "0":
            problems.append(f"{where}: exit code {rc}")
    return problems


# Selects fixtures, runs the MIPS jobs in one container, and reports mismatches
# before releasing the temporary outputs unless they were requested.
def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--compiler", type=Path, default=ROOT / "build/baka")
    parser.add_argument("--mode", choices=["opt", "noopt", "both"], default="both")
    parser.add_argument("--keep", action="store_true", help="keep the temporary directory and print its path")
    parser.add_argument("--image", default="baka-mips64el", help="Docker image with the MIPS64 toolchain and QEMU")
    parser.add_argument("cases", nargs="*", help="optional fixture names or stems")
    args = parser.parse_args()
    args.compiler = args.compiler.resolve()
    cases = collect(set(args.cases))
    if not cases:
        parser.error("no matching cases")
    tmp = Path(tempfile.mkdtemp(prefix="baka-codegen-"))
    try:
        shutil.copy(HERE / "harness.c", tmp / "harness.c")
        jobs = prepare(args, tmp, cases)
        docker = execute(args, tmp)
        if docker.returncode:
            print("docker run failed:\n" + docker.stdout + docker.stderr)
            return 1
        failures = 0
        for job in jobs:
            problems = check(tmp, job)
            print(("FAIL " if problems else "PASS ") + job["label"])
            for problem in problems:
                print("  " + problem.replace("\n", "\n  "))
            failures += bool(problems)
        print(f"{len(jobs) - failures}/{len(jobs)} passed")
    finally:
        if args.keep:
            print("kept:", tmp)
        else:
            shutil.rmtree(tmp, ignore_errors=True)
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
