import glob
import math
import os
import shutil
import sys
import time

from codecs import decode
from termcolor import colored
from wand.image import Image

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



def get_test_cases(ctx: test_ctx, target: str, MPI_enabled: bool, short_test: bool):
    cases = []
    target_path = ctx.install_dir + "/" + ctx.testing_module + "/" + target
    test_res_dir = ctx.test_tmp_dir + "/" + target + "/res"
    case_dirs = glob.glob(ctx.test_dir + "/[0-9]*/")
    for case_dir in case_dirs:
            test_tasks = []
            scenes = case_dir + "scenes.rtr"
            output_template = "output#.png"
            output = output_template.replace("#", "0")
            check_file = case_dir + output
            test_res_file = test_res_dir + "/" + output
            sequential_task = target_path + " " + scenes + " -o " + test_res_dir + "/" + output_template

            if MPI_enabled:
                nproc_arr = range((os.cpu_count() or 0) + 1)
                if short_test:
                    max_pow = int(math.log2(nproc_arr[-1]))
                    pows = range(0, max_pow)
                    short_nproc_arr = [(2 ** p) for p in pows]
                    if short_nproc_arr[-1] < nproc_arr[-1]:
                        short_nproc_arr.append(nproc_arr[-1])
                    nproc_arr = short_nproc_arr

                for nproc in nproc_arr:
                    test_tasks.append(mpirun_cmd() + " -np " + str(nproc) + " " + sequential_task)
            else:
                test_tasks.append(sequential_task)

            for test_task in test_tasks:
                cases.append(test_ctx.test_case(test_task, check_file, test_res_file))
    return cases



def calc_diff(test_case: test_ctx.test_case):
    with Image(filename=test_case.test_res_file) as res_img:
        with Image(filename=test_case.check_file) as check_img:
            diff_img, is_diff = res_img.compare(check_img,
                                                metric='fuzz',
                                                highlight='#fff',
                                                lowlight='#000')
            diff_img_name = ""
            metrics_threshold = 0.01
            if is_diff > metrics_threshold:
                diff_img_name = os.path.splitext(test_case.test_res_file)[0] + "-diff.png"
                diff_img.save(filename=diff_img_name)
                diff_msg = "diff metrics " + str(is_diff) + " with threshold " \
                        + str(metrics_threshold) \
                        + ";\ndifference was written in " + diff_img_name
                return True, diff_msg
            else:
                return False, ""



def test_target(ctx: test_ctx, target: str, MPI_enabled: bool, verbose: bool, short_test: bool):
    test_cases = get_test_cases(ctx, target, MPI_enabled, short_test)
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
        total_elapsed += elapsed
        if verbose:
            print(colored("elapsed time: ", "blue") + colored(str(elapsed) + "s", "yellow"))

        if returncode:
            test_case.diff = "'" + target + "' exited with non-zero return code"
            return test_case, str(total_elapsed)

        fail, test_case.diff = calc_diff(test_case)
        if fail:
            return test_case, str(total_elapsed)

    return False, str(total_elapsed)


def test_config(ctx: test_ctx, flt_type: str, MPI_enable: bool, verbose: bool, short_test: bool):
    ctx.set_build_task(["FLT_TYPE=" + flt_type, "PARALLEL=" + str(MPI_enable)])
    flt_type = flt_type.lower()
    build_start = time.time()
    build = ctx.build(verbose=verbose)
    build_time = time.time() - build_start
    if not build:
        test_res = test_ctx.test_case(ctx.build_task, "", "", "build failed")
        report_fail(ctx.testing_module, flt_type, MPI_enable, test_res, str(build_time))
        sys.exit(1)

    print("Testing  module '" + colored(ctx.testing_module, attrs=["bold"]) + "'")

    for target in ctx.targets:
        test_res, exec_time = test_target(ctx, target, MPI_enable, verbose, short_test)
        if test_res:
            report_fail(target, flt_type, MPI_enable, test_res, exec_time)
            sys.exit(1)
        else:
            report_success(target, flt_type, MPI_enable, exec_time)
    return False


def run_test(ctx: test_ctx,
             flt_types = [ "FLOAT", "DOUBLE" ],
             MPI_enable_arr = [ False, True ],
             verbose: bool = False,
             short_test: bool = True,
             save_temps: bool = False):
    if os.path.isdir(ctx.test_tmp_dir):
        shutil.rmtree(ctx.test_tmp_dir)
    os.mkdir(ctx.test_tmp_dir, 0o777)
    for target in ctx.targets:
        os.makedirs(ctx.test_tmp_dir + "/" + target + "/res", 0o777)

    os.chdir(ctx.test_dir)

    for MPI_enable in MPI_enable_arr:
        for flt_type in flt_types:
            test_config(ctx, flt_type, MPI_enable, verbose, short_test)

    if not save_temps:
        shutil.rmtree(ctx.test_tmp_dir)

