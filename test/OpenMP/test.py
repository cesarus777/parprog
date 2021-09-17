import os
import shutil
import sys
import time

from codecs import decode
from termcolor import colored

import test_ctx



def report_fail(target: str, test_case: test_ctx.test_case, time: str):
    print("")
    test_ctx.report({"target        " : colored(target, attrs=["bold"]),
                     "time elapsed  " : colored(time + "s", "yellow"),
                     "test          " : colored("failed", "red"),
                     "test cmd      " : colored(test_case.test_task, "cyan"),
                     "test result   " : "\n" + colored(decode(test_case.diff, "unicode_escape"), "red")})



def report_success(target: str, time: str):
    test_ctx.report({"target        " : colored(target, attrs=["bold"]),
                     "time elapsed  " : colored(time + "s", "yellow"),
                     "test          " : colored("successful", "green")})



def get_test_cases(ctx, target: str, short_test: bool = True):
    target_path = os.path.abspath(ctx.install_dir + "/" + ctx.testing_module + "/" + target)

    if target == "hello_world":
        check_file = ctx.test_tmp_dir + "/" + target + ".txt"
        if os.path.isfile(check_file):
            os.remove(check_file)
        check_file_lines = ["hello_world\n"]
        for tid in range(os.cpu_count() or 0, 0, -1):
            check_file_lines.append(str(tid - 1) + "\n")
        open(check_file, 'w').writelines(check_file_lines)
        test_res_file = ctx.test_tmp_dir + "/res/" + target + ".txt"
        task = target_path + " > " + test_res_file
        return [test_ctx.test_case(task, check_file, test_res_file)]
    #else: sum

    cases = []
    n_arr = []
    if short_test:
        n_arr = [1, 10, 100, 1000, 5432, 6543, 28356]
    else:
        n_arr = [int(10 ** (n / 3)) for n in range(0, 15)]

    for n in n_arr:
        nstr = str(n)
        check_file = ctx.test_tmp_dir + "/" + nstr + ".txt"
        if not os.path.isfile(check_file):
            open(check_file, 'w').write(str(int(n * (n + 1) // 2)))
        test_res_file = ctx.test_tmp_dir + "/res/" + nstr + ".txt"
        task = target_path + " " + nstr + " > " + test_res_file
        cases.append(test_ctx.test_case(task, check_file, test_res_file))

    return cases



def test_target(ctx, target: str, verbose: bool, short_test: bool = True):
    test_cases = get_test_cases(ctx, target, short_test)
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

        res   = open(test_case.test_res_file, 'r').read()
        check = open(test_case.check_file,    'r').read()

        diff = res == check

        if diff:
            test_case.diff = "expected: " + check + "got: " + res
            return test_case, str(total_elapsed)

    return False, str(total_elapsed)



def test_config(ctx, verbose: bool = False, short_test: bool = True):
    ctx.set_build_task([])
    build_start = time.time()
    build = ctx.build(verbose=verbose)
    build_time = time.time() - build_start
    if not build:
        test_res = test_ctx.test_case(ctx.build_task, "", "", "build failed")
        report_fail(ctx.testing_module, test_res, str(build_time))
        sys.exit(1)

    print("Testing  module '" + colored(ctx.testing_module, attrs=["bold"]) + "'")

    for target in ctx.targets:
        test_res, exec_time = test_target(ctx, target, verbose, short_test)
        if test_res:
            report_fail(target, test_res, exec_time)
            sys.exit(1)
        else:
            report_success(target, exec_time)



def run_test(ctx, verbose: bool = False, short_test: bool = True, save_temps: bool = False):
    if os.path.isdir(ctx.test_tmp_dir):
        shutil.rmtree(ctx.test_tmp_dir)

    mode = 0o777
    os.makedirs(ctx.test_tmp_dir + "/res", mode)
    os.chdir(ctx.test_dir)

    test_config(ctx, verbose, short_test)

    if not save_temps:
        shutil.rmtree(ctx.test_tmp_dir)

