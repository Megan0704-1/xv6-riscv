# obj-y is used for object files that sould be compiled into the kernel image directly. (would be treated like built-in kernel object)
# `:=`is an immediate assignment replacing whatever was there with the right-hand side.
obj-y := my_syscall.o

clean:
	rm -f *.o *.cmd .modules.order.cmd .*.o.cmd .*.a.cmd
	rm -f built-in.a
