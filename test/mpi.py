from subprocess import run, PIPE

def has_Open_MPI():
    mpi_check = run(["mpirun", "--version"], stdout=PIPE, text=True)
    if mpi_check.returncode:
        return False
    if mpi_check.stdout == None:
        return False
    if "Open MPI" in mpi_check.stdout:
        return True
    return False

def mpirun_cmd():
    return "mpirun --use-hwthread-cpus" if has_Open_MPI() else "mpirun"

