import os
import subprocess
import sys
from shutil import which

if which("pasim") is None:
    print("Patmos simulator 'pasim' could not be found.")
    sys.exit(1)

first_exec_arg_index = 9
if len(sys.argv) <= (first_exec_arg_index + 1):
    print("Must have at least 2 execution arguments but was:", sys.argv[first_exec_arg_index:])
    sys.exit(1)
   
# Parse arguments
   
# Takes the path to one LLVM build binary and extracts the directory
bin_dir = os.path.dirname(sys.argv[1])

# The source file to test
source_to_test = sys.argv[2]

# Substitute source file
substitute_source = sys.argv[3]

if substitute_source != "":
    source_to_test = substitute_source
    
# The object file of the program
compiled = sys.argv[4]   

# The (potential) debug file
debug_file = compiled + ".debug"

# The object file of the start function to be linked with the program
start_function = sys.argv[5]

# The object file of the memory access compensation function to be linked with the program
compensation_function = sys.argv[6]

# When using single-path, which function is the root single-path function
sp_root = sys.argv[7]

# Whether LLC should be run with debuggin enable
with_debug = sys.argv[8] == "true"

# The first execution argument
exec_arg = sys.argv[first_exec_arg_index]

# It cleans the given pasim statistics
# leaving only the stats needed to ensure two run of a singlepath
# program are identical (execution-wise).
def pasim_stat_clean(stats):
    input = stats.splitlines()
    output = ""
    currentLine = 0;
    #Find the instruction statistics
    while input[currentLine].strip() != "Instruction Statistics:":
        if input[currentLine].strip() == "Pasim options:":
            raise ValueError("No pasim statistics given.")
        currentLine+=1
    currentLine +=2 #Discard the found header and the following line
    
    #output cleaned instruction statistics
    while not input[currentLine].strip().startswith("all:"):
        split_line = input[currentLine].split()
        name = split_line[0]
        fetch_count = int(split_line[1]) + int(split_line[4])
        output += name + " " + str(fetch_count)+"\n"
        currentLine+=1
    currentLine+=1 # Discard the "all:"
    
    #Find and output cycle count
    while not input[currentLine].strip().startswith("Cycles:"):
        currentLine+=1
    split = input[currentLine].split()
    output += split[0] + " " + split[1] + "\n"
    currentLine+=1
    
    #Find profiling information
    while not input[currentLine].strip().startswith("Profiling information:"):
        currentLine+=1
    #Discard the header and next 3 lines, which are just table headers
    currentLine+=4
            
    #Output how many times each function is called
    while True:
        line = input[currentLine].strip()
        currentLine+=1
        if line.startswith("<"):
            currentLine+=1 #Discard next line
            count = input[currentLine].strip().split()[0]
            currentLine+=1
            output += line[1:].split(">")[0] + "(): " + count + "\n"
        else:
            #Not part of the profiling
            break;
    
    return output
    
# Executes the given program (arg 1), running statistics on the given function (arg 2).
# Argument 3 is the execution arguments (see top of file for description).
# Tests that the output of the program match the expected output. If not, reports an error.
# Returns the cleaned statistics.
def execute_and_stat(program, args, pasim_args):
	# Split the execution argument into input and expected output.
    split = args.split("=", 1)
    input = split[0]
    expected_out = split[1]
    
    # The final name of the ELF to execute
    exec_name = program + input
    
    # Final generation of ELF with added input
    if subprocess.run(["ld.lld", "-nostdlib", "-static", "-o", exec_name, program, 
        "--defsym", "input=" + input]).returncode != 0:
        return True, args + "\nFailed to generate executable from '" + program + "' for argument '" + input + "'"
     
    try:
        pasim_result = subprocess.run(["pasim", exec_name, "-V"] + pasim_args.split(), 
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=10)
    except subprocess.TimeoutExpired:
        return True, "\nThe execution of '" + program + "' for input argument '" + input + "' timed out"
    
    program_output=pasim_result.stdout
    pasim_stats=pasim_result.stderr
    
    if int(expected_out) != pasim_result.returncode:
        sys.stderr.write("The execution of '" + program + "' for input argument '" + input + "' gave the wrong output through stdout.\n")
        sys.stderr.write("-------------------- Expected --------------------\n")
        sys.stderr.write(expected_out + "\n")
        sys.stderr.write("--------------------- Actual ---------------------\n")
        sys.stderr.write(str(pasim_result.returncode) + "\n")
        sys.stderr.write("--------------------- stdout ---------------------\n")
        sys.stderr.write(program_output + "\n")
        sys.stderr.write("--------------------- stderr ---------------------\n")
        sys.stderr.write(pasim_stats + "\n")
        sys.stderr.write("--------------------------------------------------\n")
        return True, ""
    
    # Clean 'pasim's statistics
    try: 
        return False, pasim_stat_clean(pasim_stats)
    except ValueError:
		# If cleaning failed it means the stderr is not statistics and some other
		# error was printed
        sys.stderr.write("--------------------- stderr ---------------------\n")
        sys.stderr.write(pasim_stats + "\n")
        sys.stderr.write("--------------------------------------------------\n")
        return True, ""
       
# Compile and test using the given LLC arguments.
def compile_and_test(llc_args, pasim_args):
    def throw_error(*msgs):
        for msg in msgs:
            print(msg, end = '')
        print("\nStart file: ", os.path.basename(start_function))
        print("LLC args: ", llc_args)
        print("Pasim args: ", pasim_args)
        if with_debug:
            print("Debug file: ", debug_file)
        sys.exit(1)
        
    using_singlepath = "-mpatmos-singlepath=" in llc_args
    using_cet = "-mpatmos-enable-cet" in llc_args
        
    # Link start function with program
    if subprocess.run([bin_dir+"/llvm-link", start_function, source_to_test, "-o", compiled]).returncode != 0:
        throw_error("Failed to link '", source_to_test, "' and '", start_function, "'")
    
    # Link memory access compensation
    if using_cet:
        if subprocess.run([bin_dir+"/llvm-link", compensation_function, compiled, "-o", compiled]).returncode != 0:
            throw_error("Failed to link '", compiled, "' and '", compensation_function, "'")
    
    # Compile into object file (not ELF yet)
    llc_compile_arg_list =[bin_dir+"/llc", compiled] + llc_args.split() + ["-filetype=obj", "-o", compiled]
    if with_debug:
        llc_compile_arg_list = llc_compile_arg_list + ["--debug", "--print-after-all"]
        stderr_cfg = open(debug_file, "w", 1)
    else:
        stderr_cfg = None

    if subprocess.run(llc_compile_arg_list, stderr=stderr_cfg).returncode != 0:
        throw_error("Failed to compile '", source_to_test, "'")
     
    # Run the first execution argument on its own,
    # such that its stats result can be compared to
    # all other executions
    first_stats_failed, first_stats=execute_and_stat(compiled, exec_arg, pasim_args)
    if first_stats_failed:
        throw_error(first_stats)

      
    # Run the rest of the execution arguments.
    # For each one, compare to the first. If they all
    # are equal to the first, they must also be equal to each other,
    # so we don't need to compare them to each other.
    for i in sys.argv[first_exec_arg_index:]:
            
        rest_failed, rest_stats=execute_and_stat(compiled, i, pasim_args)
        if rest_failed:
            throw_error()
        
        if using_singlepath:
            if first_stats != rest_stats:
                import difflib
                sys.stderr.writelines(difflib.context_diff(first_stats.split("\n"), rest_stats.split("\n")))
                throw_error("The execution of '", compiled, "' for execution arguments '", exec_arg, "' and '", i, "' weren't equivalent")

# Compile and test all compinations in the given matrix
# The matrix is a list of either string pairs or nested matrices.
# A string pair defines argument to pass to llc and pasim respectively.
# If only 1 string is given, pasim's string is implicitly empty.
#
# For each pair, the string are appended to the given llc and pasim argument strings
# and then if there are any matrices the resulting strings are used in a recursive call.
#
# string pairs in a matrix define independent test configs that are then each merged with any nested
# matrices.
def compile_and_test_matrix(llc_args, pasim_args, matrix):
    # Collect strings and lists
    options=[]
    inner=[]
    for arg in matrix:
        if type(arg) is tuple:
            options.append(arg)
        elif type(arg) is str:
            options.append((arg, ""))
        elif type(arg) is list:
            inner.append(arg)
        else:
            throw_error("Invalid matrix 1")
    
    for (llc_next, pasim_next) in options:
        llc_merged = llc_args + " " + llc_next
        pasim_merged = pasim_args + " " + pasim_next
        if len(inner) == 0:
            compile_and_test(llc_merged, pasim_merged)
        else:
            for inn in inner:
                compile_and_test_matrix(llc_merged, pasim_merged, inn)

compile_and_test_matrix("", "", [
    # Optimization levels
    "", "-O1", 
    "-O2", "-O2 -mpatmos-disable-vliw=false",
    # We try low subfuction size to ensure the splitter works too 
    # (without needing to make tests with big functions)
    "-O2 --mpatmos-max-subfunction-size=64", 
    [
        # We add this indirection so that commenting out the following line will remove all traditional tests
        "",
        [
            #Traditional
            "",
            # Traditional code with PML output, just to make sure we can
            # output it without errors. Testing the output is not done.
            "-mserialize-pml=" + compiled + ".pml -mserialize-pml-functions=" + sp_root
        ]
    ],
    [
        # Single-Path
        "-mpatmos-singlepath=" + sp_root,
        [
            "-mpatmos-disable-singlepath-scheduler-equivalence-class",
            "", 
            ("-mpatmos-enable-permissive-dual-issue", "--permissive-dual-issue"),
            "-mpatmos-disable-pseudo-roots", "-mpatmos-disable-countless-loops",
            [
                ("", "-D ideal") # Ignore variability from data cache
            ],
            # Constant execution time
            [
                ("", "-D lru2"), # Non-ideal cache will give execution-time variability of code is not constant
                [
                    "-mpatmos-enable-cet=opposite",
                    "-mpatmos-enable-cet=counter",
                    "-mpatmos-enable-cet=hybrid",
                ]
            ],
        ]
    ],
])

# Success
sys.exit(0)
