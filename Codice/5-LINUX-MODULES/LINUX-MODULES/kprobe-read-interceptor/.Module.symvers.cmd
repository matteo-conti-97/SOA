cmd_/home/francesco/kernel-modules/LINUX-MODULES/kprobe-read-interceptor/Module.symvers := sed 's/\.ko$$/\.o/' /home/francesco/kernel-modules/LINUX-MODULES/kprobe-read-interceptor/modules.order | scripts/mod/modpost -m -a  -o /home/francesco/kernel-modules/LINUX-MODULES/kprobe-read-interceptor/Module.symvers -e -i Module.symvers   -T -
