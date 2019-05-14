
██████╗ ██╗   ██╗███╗   ██╗ █████╗ ███╗   ███╗██╗ ██████╗    ██████╗ ██╗███╗   ██╗██████╗  █████╗ ██╗   ██╗
██╔══██╗╚██╗ ██╔╝████╗  ██║██╔══██╗████╗ ████║██║██╔════╝    ██╔══██╗██║████╗  ██║██╔══██╗██╔══██╗╚██╗ ██╔╝
██║  ██║ ╚████╔╝ ██╔██╗ ██║███████║██╔████╔██║██║██║         ██████╔╝██║██╔██╗ ██║██████╔╝███████║ ╚████╔╝
██║  ██║  ╚██╔╝  ██║╚██╗██║██╔══██║██║╚██╔╝██║██║██║         ██╔══██╗██║██║╚██╗██║██╔══██╗██╔══██║  ╚██╔╝
██████╔╝   ██║   ██║ ╚████║██║  ██║██║ ╚═╝ ██║██║╚██████╗    ██████╔╝██║██║ ╚████║██║  ██║██║  ██║   ██║
╚═════╝    ╚═╝   ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝ ╚═════╝    ╚═════╝ ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝

████████╗██████╗  █████╗ ███╗   ██╗███████╗██╗      █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██║     ██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
   ██║   ██████╔╝███████║██╔██╗ ██║███████╗██║     ███████║   ██║   ██║██║   ██║██╔██╗ ██║
   ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██║     ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
   ██║   ██║  ██║██║  ██║██║ ╚████║███████║███████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝


███████╗██╗  ██╗    ██████╗ 
██╔════╝╚██╗██╔╝    ╚════██╗
█████╗   ╚███╔╝      █████╔╝
██╔══╝   ██╔██╗     ██╔═══╝ 
███████╗██╔╝ ██╗    ███████╗
╚══════╝╚═╝  ╚═╝    ╚══════╝
                                                         
translation 
EX2
Submitters - Dani Sulam 302560735 and Hanan Hazani 203704424

--- Overview ---
This tool collects statistics about loops in a given binary (and shared objects) and output them to a file loop-count.csv
Definitions:
A background edge traversal counts as a loop iteration.
A forward edge (NOT to a follow through BBL of a loop condition BBL) traversal counts as a loop invocation

We do not count invocations where a loop was escaped during its body execution.

--- How to compile ex2.so ---
Execute the following in terminal:

cd path_to_hw2/src/ && make PIN_ROOT:=/path/to/pin3.7_root obj-intel64/ex2.so

--- How to run the tool ---
Here is an example of how to run the tool with the bzip2 program:
cd /mnt/hgfs/HW/hw2/src/tests/bzip2/ && .../pin-3.7-97619-g0d0c92f4f-gcc-linux/pin -t .../hw2/src/obj-intel64/ex2.so  -- ./bzip2 -k -f ./input.txt 
