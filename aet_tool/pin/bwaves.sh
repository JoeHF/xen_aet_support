#!/bin/sh
cd /new2/benchmarks/SPEC2006/benchspec/CPU2006/410.bwaves/run/run_base_ref_none.0000
/root/pin/pin -t /root/pin/source/tools/mine/obj-intel64/pinatrace_do_once.so -- ./bwaves_base.none 
sh scp_file.sh bwaves
cd -

