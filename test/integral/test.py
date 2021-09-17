import math
import os
import shutil
import sys
import time

from codecs import decode
from termcolor import colored

import test_ctx



def report_fail(target: str, flt_type: str, test_case: test_ctx.test_case, time: str):
    print("")
    test_ctx.report({"target        " : colored(target, attrs=["bold"]),
                     "with flt_type " : colored(flt_type, "yellow"),
                     "time elapsed  " : colored(time + "s", "yellow"),
                     "test          " : colored("failed", "red"),
                     "test cmd      " : colored(test_case.test_task, "cyan"),
                     "test result   " : "\n" + colored(decode(test_case.diff, "unicode_escape"), "red")})



def report_success(target: str, flt_type: str, time: str):
    test_ctx.report({"target        " : colored(target, attrs=["bold"]),
                     "with flt_type " : colored(flt_type, "yellow"),
                     "time elapsed  " : colored(time + "s", "yellow"),
                     "test          " : colored("successful", "green")})



def get_test_cases(ctx, target: str, flt_type: str, short_test: bool):
    cases = []
    target_path = os.path.abspath(ctx.install_dir + "/" + ctx.testing_module + "/" + target)

    nproc_arr = range(1, (os.cpu_count() or 0) + 1)
    if short_test:
        max_pow = int(math.log2(nproc_arr[-1]))
        pows = range(0, max_pow)
        short_nproc_arr = [(2 ** p) for p in pows]
        if short_nproc_arr[-1] < nproc_arr[-1]:
            short_nproc_arr.append(nproc_arr[-1])
        nproc_arr = short_nproc_arr

    for n in nproc_arr:
        check_file = ctx.test_dir + "/" + flt_type + ".txt"
        test_res_file = ctx.test_tmp_dir + "/res/" + flt_type + ".txt"
        task = target_path + " -j=" + str(n) + " > " + test_res_file
        cases.append(test_ctx.test_case(task, check_file, test_res_file))

    return cases



def test_target(ctx, target: str, flt_type: str, verbose: bool, short_test: bool):
    test_cases = get_test_cases(ctx, target, flt_type, short_test)
    total_elapsed = 0

    for test_case in test_cases:
        if not os.access(test_case.check_file, os.R_OK):
            test_case.diff = __file__ + ": file '" + test_case.check_file + "' not found"
            return test_case, "0"

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

        res   = open(test_case.test_res_file, 'r').read().replace('\n', '')
        check = open(test_case.check_file,    'r').read().replace('\n', '')

        res_value   = float(res)
        check_value = float(check)

        diff = abs(check_value - res_value)

        epsilon = 1.5e-6 if flt_type == "float" else 1.5e-15

        if diff > epsilon:
            test_case.diff = res + " != " + check
            return test_case, str(total_elapsed)

    return False, str(total_elapsed)



def test_config(ctx, flt_type: str, verbose: bool, short_test: bool):
    ctx.set_build_task(["FLT_TYPE=" + flt_type])
    flt_type = flt_type.lower()
    build_start = time.time()
    build = ctx.build(verbose=verbose)
    build_time = time.time() - build_start
    if not build:
        test_res = test_ctx.test_case(ctx.build_task, "", "", "build failed")
        report_fail(ctx.testing_module, flt_type, test_res, str(build_time))
        sys.exit(1)

    print("Testing  module '" + colored(ctx.testing_module, attrs=["bold"]) + "'")

    for target in ctx.targets:
        test_res, exec_time = test_target(ctx, target, flt_type, verbose, short_test)
        if test_res:
            report_fail(target, flt_type, test_res, exec_time)
            sys.exit(1)
        else:
            report_success(target, flt_type, exec_time)



def run_test(ctx,
             flt_types = [ "FLOAT", "DOUBLE" ],
             verbose: bool = False,
             short_test: bool = True,
             save_temps: bool = False):
    if os.path.isdir(ctx.test_tmp_dir):
        shutil.rmtree(ctx.test_tmp_dir)

    mode = 0o777
    os.makedirs(ctx.test_tmp_dir + "/res", mode)
    os.chdir(ctx.test_dir)

    for flt_type in flt_types:
        test_config(ctx, flt_type, verbose, short_test)

    if not save_temps:
        shutil.rmtree(ctx.test_tmp_dir)

