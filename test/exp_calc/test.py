import math
import os
import shutil
import subprocess
import sys
import time

from codecs import decode
from termcolor import colored

from mpi import mpirun_cmd

import test_ctx



def report_fail(target: str, flt_type: str, MPI_enabled: bool, test_case: test_ctx.test_case, time: str):
    print("")
    test_ctx.report({"target        " : colored(target, attrs=["bold"]),
                     "with flt_type " : colored(flt_type, "yellow"),
                     "MPI           " : colored("enabled", "green") if MPI_enabled else colored("disabled", "red"),
                     "time elapsed  " : colored(time + "s", "yellow"),
                     "test          " : colored("failed", "red"),
                     "test cmd      " : colored(test_case.test_task, "cyan"),
                     "test result   " : "\n" + colored(decode(test_case.diff, "unicode_escape"), "red")})



def report_success(target: str, flt_type: str, MPI_enabled: bool, time: str):
    test_ctx.report({"target        " : colored(target, attrs=["bold"]),
                     "with flt_type " : colored(flt_type, "yellow"),
                     "MPI           " : colored("enabled", "green") if MPI_enabled else colored("disabled", "red"),
                     "time elapsed  " : colored(time + "s", "yellow"),
                     "test          " : colored("successful", "green")})



def get_test_cases(ctx, target, flt_type, MPI_enabled, short_test: bool):
    sequential_cases = []
    target_path = os.path.abspath(ctx.install_dir + "/" + ctx.testing_module + "/" + target)

    if target == "parallel_with_long_arithmetic":
        for n in [ "10", "100", "1000", "10000" ]:
            check_file = ctx.test_dir + "/" + target + "/e" + n + ".txt"
            test_res_file = ctx.test_tmp_dir + "/res/" + target  + "/e" + n + ".txt"
            sequential_task = target_path + " " + n + " > " + test_res_file
            sequential_cases.append(test_ctx.test_case(sequential_task, check_file, test_res_file))
    else:
        check_file = ctx.test_dir + "/" + target + "/" + flt_type + ".txt"
        test_res_file = ctx.test_tmp_dir + "/res/" + target  + "/" + flt_type + ".txt"
        sequential_task = target_path + " > " + test_res_file
        sequential_cases.append(test_ctx.test_case(sequential_task, check_file, test_res_file))

    if MPI_enabled:
        cases = []
        nproc_arr = range((os.cpu_count() or 0) + 1)
        if short_test:
            max_pow = int(math.log2(nproc_arr[-1]))
            pows = range(0, max_pow)
            short_nproc_arr = [(2 ** p) for p in pows]
            if short_nproc_arr[-1] < nproc_arr[-1]:
                short_nproc_arr.append(nproc_arr[-1])
            nproc_arr = short_nproc_arr

        for nproc in nproc_arr:
            for sequential_case in sequential_cases:
                cases.append(test_ctx.test_case(mpirun_cmd() + " -np " + str(nproc) + " " +
                                                sequential_case.test_task, sequential_case.check_file, sequential_case.test_res_file))

        return cases
    else:
        return sequential_cases



def test_target(ctx, target: str, flt_type: str, MPI_enabled: bool, verbose: bool, short_test: bool):
    test_cases = get_test_cases(ctx, target, flt_type, MPI_enabled, short_test)
    total_elapsed = 0.0

    for test_case in test_cases:
        if not os.access(test_case.check_file, os.R_OK):
            test_case.diff = __file__ + ": file '" + test_case.check_file + "' not found"
            return test_case, str(total_elapsed)

        if verbose:
            message = colored("running: ", "blue") + colored(test_case.test_task, "cyan")
            print(message)

        start = time.time()
        returncode = os.system(test_case.test_task)
        elapsed = time.time() - start
        total_elapsed += float(elapsed)
        if verbose:
            print(colored("elapsed time: ", "blue"), colored(str(elapsed) + "s", "yellow"))

        if returncode:
            test_case.diff = "'" + target + "' exited with non-zero return code"
            return test_case, str(total_elapsed)

        diff_run = subprocess.run(["diff", test_case.check_file, test_case.test_res_file],
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)
        if diff_run.returncode:
            test_case.diff = diff_run.stdout
            return test_case, str(total_elapsed)

    return False, str(total_elapsed)



def test_config(ctx, flt_type: str, MPI_enable: bool, verbose: bool, short_test: bool):
    ctx.set_build_task(["FLT_TYPE=" + flt_type,
                        "PARALLEL=" + ("ON" if MPI_enable else "OFF")])
    flt_type = flt_type.lower()
    if not ctx.build(verbose=verbose):
        test_res = test_ctx.test_case(ctx.build_task, "", "", "build failed")
        report_fail(ctx.testing_module, flt_type, MPI_enable, test_res, "0")
        sys.exit(1)

    print("Testing  module '" + colored(ctx.testing_module, attrs=["bold"]) + "'")

    for target in ctx.targets:
        if MPI_enable and target == "sequential":
            continue

        test_res, time = test_target(ctx, target, flt_type, MPI_enable, verbose, short_test)
        if test_res:
            report_fail(target, flt_type, MPI_enable, test_res, time)
            sys.exit(1)
        else:
            report_success(target, flt_type, MPI_enable, time)



def run_test(ctx,
             flt_types = [ "FLOAT", "DOUBLE" ],
             MPI_enable_arr = [ False, True ],
             verbose: bool = False,
             short_test: bool = True,
             save_temps: bool = False):
    if os.path.isdir(ctx.test_tmp_dir):
        shutil.rmtree(ctx.test_tmp_dir)

    mode = 0o777
    os.makedirs(ctx.test_tmp_dir + "/res", mode)
    for target in ctx.targets:
        os.mkdir(ctx.test_tmp_dir + "/res/" + target, mode)

    os.chdir(ctx.test_dir)

    if len(ctx.targets) == 1 and ctx.targets[0] == "sequential":
        MPI_enable_arr = [False]

    for MPI_enable in MPI_enable_arr:
        for flt_type in flt_types:
            test_config(ctx, flt_type, MPI_enable, verbose, short_test)

    if not save_temps:
        shutil.rmtree(ctx.test_tmp_dir)

