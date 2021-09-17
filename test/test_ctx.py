import os
import subprocess
import sys

from termcolor import colored



def report(params: dict):
    for key, val in params.items():
        print(key + ":  " + val)
    print("")



class test_case:
    def __init__(self, test_task: str, check_file: str, test_res_file: str, diff: str=""):
        self.test_task = test_task
        self.check_file = check_file
        self.test_res_file = test_res_file
        self.diff = diff



class test_ctx(object):

    def __init__(self, testing_module, targets, root="", build_dir = "/build", install_dir = "/install"):
        self.testing_module = testing_module
        self.targets        = targets

        self.repo_root_dir = root if len(root) else os.environ.get("PARPROG_ROOT_DIR", "")
        if not os.path.isdir(self.repo_root_dir):
            print(colored("error: ", "red", attrs=["bold"]) + __file__ + ": env var PARPROG_ROOT_DIR is not a dir")
            sys.exit(1)

        self.repo_root_dir = os.path.abspath(self.repo_root_dir)

        self.test_dir     = self.repo_root_dir + "/test/" + self.testing_module
        self.test_tmp_dir = self.test_dir      + "/tmp"
        self.build_dir    = self.test_tmp_dir  + build_dir
        self.install_dir  = self.test_tmp_dir  + install_dir

        #self.build_task  = self.repo_root_dir  + "/support/install.sh"
        #self.build_task += " --targets "       + self.testing_module
        #self.build_task += " --src-dir "       + self.repo_root_dir
        #self.build_task += " --build-dir "     + self.build_dir
        #self.build_task += " --install-dir "   + self.install_dir

        self.build_task = []
        self.build_task.append(self.repo_root_dir  + "/support/install.sh")
        self.build_task.append("--targets")
        self.build_task.append(self.testing_module)
        self.build_task.append("--src-dir")
        self.build_task.append(self.repo_root_dir)
        self.build_task.append("--build-dir")
        self.build_task.append(self.build_dir)
        self.build_task.append("--install-dir")
        self.build_task.append(self.install_dir)



    def set_build_task(self,
                       definitions = [],
                       build_type = "Release"):

        self.build_task = []
        self.build_task.append(self.repo_root_dir  + "/support/install.sh")
        for definition in definitions:
            self.build_task.append("-D " + definition)

        self.build_task.append("--targets")
        self.build_task.append(self.testing_module)
        self.build_task.append(" --build-type")
        self.build_task.append(build_type)
        self.build_task.append("--src-dir")
        self.build_task.append(self.repo_root_dir)
        self.build_task.append("--build-dir")
        self.build_task.append(self.build_dir)
        self.build_task.append("--install-dir")
        self.build_task.append(self.install_dir)

        return self.build_task



    def build(self, verbose: bool = False):
        print("Building module '" + colored(self.testing_module, attrs=["bold"]) + "'")
        returncode = 1

        if verbose:
            build_task = " ".join(self.build_task)
            print(colored(build_task, "cyan"))
            returncode = os.system(build_task)
        else:
            build_res = subprocess.run(self.build_task, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            returncode = build_res.returncode

        return False if returncode else True

