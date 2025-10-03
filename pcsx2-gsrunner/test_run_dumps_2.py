import argparse
import glob
import sys
import os
import subprocess
import multiprocessing
from pathlib import Path
from functools import partial
import platform

def get_gs_name(path):
    lpath = path.lower()

    for extension in [".gs", ".gs.xz", ".gs.zst"]:
        if lpath.endswith(extension):
            return os.path.basename(path)[:-len(extension)]

    return None


def run_regression_test(runner, dumpdir, renderer, upscale, renderhacks, gspath, nbatches, batch_id):
    args = [runner]
    
    args.extend(["-batch"])

    args.extend(["-nbatches", str(nbatches)])
    
    args.extend(["-batch-id", str(batch_id)])

    if renderer is not None:
        args.extend(["-renderer", renderer])

    if upscale != 1.0:
        args.extend(["-upscale", str(upscale)])

    if renderhacks is not None:
        args.extend(["-renderhacks", renderhacks])

    args.extend(["-dumpdir", dumpdir])
    args.extend(["-logfile", os.path.join(dumpdir, "emulog.txt")])

    # loop a couple of times for those stubborn merge/interlace dumps that don't render anything
    # the first time around
    args.extend(["-loop", "2"])

    # disable shader cache for parallel runs, otherwise it'll have sharing violations
    if nbatches > 1:
        args.append("-noshadercache")

    # run surfaceless, we don't want tons of windows popping up
    args.append("-surfaceless");

    # disable output console entirely
    environ = os.environ.copy()
    environ["PCSX2_NOCONSOLE"] = "1"

    creationflags = 0
    # Set low priority by default
    if platform.system() == "Windows":
        creationflags = 0x00004000  # BELOW_NORMAL_PRIORITY_CLASS
    elif platform.system() in ["Linux", "Darwin"]:
        try:
            os.nice(10)  # lower priority
        except OSError:
            pass

    args.append("--")
    args.append(gspath)

    #print("Running '%s'" % (" ".join(args)))
    subprocess.run(args, env=environ, stdin=subprocess.DEVNULL, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, creationflags=creationflags)


def run_regression_tests(runner, gsdir, dumpdir, renderer, upscale, renderhacks, parallel=1):
    try:
        os.makedirs(dumpdir)
    except FileExistsError:
        pass

    if parallel <= 1:
        run_regression_test(runner, dumpdir, renderer, upscale, renderhacks, gsdir, 1, 0)
    else:
        print("Processing on %u processors" % (parallel))
        func = partial(run_regression_test, runner, dumpdir, renderer, upscale, renderhacks, gsdir, parallel)
        pool = multiprocessing.Pool(parallel)
        for i, _ in enumerate(pool.imap_unordered(func, range(parallel), chunksize=1)):
            print("Process %u finished" % (i))
        pool.close()

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate frame dump images for regression tests")
    parser.add_argument("-runner", action="store", required=True, help="Path to PCSX2 GS runner")
    parser.add_argument("-gsdir", action="store", required=True, help="Directory containing GS dumps")
    parser.add_argument("-dumpdir", action="store", required=True, help="Base directory to dump frames to")
    parser.add_argument("-renderer", action="store", required=False, help="Renderer to use")
    parser.add_argument("-upscale", action="store", type=float, default=1, help="Upscaling multiplier to use")
    parser.add_argument("-renderhacks", action="store", required=False, help="Enable HW Rendering hacks")
    parser.add_argument("-parallel", action="store", type=int, default=1, help="Number of processes to run")

    args = parser.parse_args()

    if not run_regression_tests(args.runner, os.path.realpath(args.gsdir), os.path.realpath(args.dumpdir), args.renderer, args.upscale, args.renderhacks, args.parallel):
        sys.exit(1)
    else:
        sys.exit(0)

# -runner C:\Users\tchan\Code\PS2\pcsx2-fork-tjnotjt\bin\pcsx2-gsrunnerx64-dbg.exe -gsdir "C:\Users\tchan\Desktop\pcsx2_gs_dumps\accline" -dumpdir "C:\Users\tchan\Desktop\ps2_debug\test" -renderer d3d11 -upscale 1 -parallel 4

