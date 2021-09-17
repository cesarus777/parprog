#! /usr/bin/env python3

import sys

from termcolor import colored

import test_ctx
from exp_calc   import test as exp_calc
from integral   import test as integral
from OpenMP     import test as OpenMP
from ray_tracer import test as ray_tracer



dump_config = False
save_temps = False
short_test = True
verbose = False
options = []
modules = {"exp_calc"   : ["sequential", "parallel_with_floats", "parallel_with_long_arithmetic"],
           "integral"   : ["integral"],
           "OpenMP"     : ["hello_world", "sum"],
           "ray_tracer" : ["ray_tracer"]}



class opt:
    def __init__(self, vals, description: str, process_func):
        self.vals = vals
        self.desc = description
        self.process_func = process_func

    def is_this_opt(self, arg):
        return arg in self.vals

    def process(self):
        return self.process_func()



class opt_dump_config(opt):
    def __init__(self):
        self.vals = ["-d", "--dump-config"]
        self.desc = "Dump test config before start"

    def process(self):
        global dump_config
        dump_config = True



class opt_help(opt):
    def __init__(self):
        self.vals = ["-h", "--help"]
        self.desc = "Print help message"

    def process(self):
        global options
        opt_vals_width = 20
        print("Usage: test.py [OPTION(s)]")
        print("Options:")
        for option in options:
            opt_vals_str = "\t" +  ", ".join(option.vals)
            print(opt_vals_str + " " * (opt_vals_width - len(opt_vals_str)) + option.desc)
        sys.exit(0)


class opt_long_test(opt):
    def __init__(self):
        self.vals = ["-l", "--long"]
        self.desc = "Run more test cases"

    def process(self):
        global short_test
        short_test = False



class opt_module(opt):
    def __init__(self):
        self.vals = ["-m", "--module"]
        self.desc = "Test only specified module"

    def process(self):
        global modules
        for module_opt in self.vals:
            if module_opt in sys.argv:
                module_name_idx = sys.argv.index(module_opt) + 1

                if module_name_idx >= len(sys.argv):
                    print(colored("error", "red", attrs=["bold"]) + ": no module name was provided after " + module_opt)
                    sys.exit(2)

                module_name = sys.argv[module_name_idx]
                if module_name in modules:
                    targets = modules[module_name]
                    modules = dict.fromkeys([module_name], targets)
                else:
                    print(__file__ + ": module '" + colored(module_name, attrs=["bold"]) + "' can't be tested")
                    sys.exit(1)

                return



class opt_save_temps(opt):
    def __init__(self):
        self.vals = ["--save-temps"]
        self.desc = "Do not delete temporary files"

    def process(self):
        global save_temps
        save_temps = True


class opt_target(opt):
    def __init__(self):
        self.vals = ["-t", "--target"]
        self.desc = "Test only specified target"

    def process(self):
        global modules
        for opt_target in self.vals:
            if opt_target in sys.argv:
                target_name_idx = sys.argv.index(opt_target) + 1

                if target_name_idx >= len(sys.argv):
                    print(colored("error", "red", attrs=["bold"]) + ": no target name was provided after " + opt_target)
                    sys.exit(2)

                target_name = sys.argv[target_name_idx]
                for module in modules:
                    if target_name in modules[module]:
                        modules = dict.fromkeys([module], [target_name])
                        break
                    else:
                        print(__file__ + ": target '" + colored(target_name, attrs=["bold"]) + "' can't be tested")
                        sys.exit(1)

                return



class opt_verbose(opt):
    def __init__(self):
        self.vals = ["-v", "--verbose"]
        self.desc = "Be verbose"

    def process(self):
        global verbose
        verbose = True



#options = [opt_dump_config, opt_help, opt_long_test, opt_module, opt_save_temps, opt_target, opt_verbose]
options = [opt_dump_config(),
           opt_help(),
           opt_long_test(),
           opt_module(),
           opt_save_temps(),
           opt_target(),
           opt_verbose()]



if __name__ == "__main__":
    for arg in sys.argv[1:]:
        for option in options:
            if option.is_this_opt(arg):
                option.process()

    if dump_config:
        print("short_test =", short_test)
        print("verbose =", verbose)
        print("testing modules and tartgets:", modules)

    for module, targets in modules.items():
        try:
            module_ctx = module + "_ctx"
            exec(module + ".run_test(test_ctx.test_ctx(module, targets), \
                 verbose=verbose, short_test=short_test, save_temps=save_temps)")
        except KeyboardInterrupt:
            print("\nmodule '" + colored(module, attrs=["bold"]) + "' testing was interrupted by user")
            sys.exit(1)

