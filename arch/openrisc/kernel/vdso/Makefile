obj-y += vdso-syscall.o

vobjs-y := note.o vclock_gettime.o

$(obj)/vdso-syscall.o: $(obj)/vdso.so

# Teach kbuild about targets
targets += vdso.so vdso.lds

# The DSO images are built using a special linker script
quiet_cmd_syscall = SYSCALL $@
      cmd_syscall = $(CC) -fPIC -nostdlib $(SYSCFLAGS_$(@F)) \
			   -Wl,-T,$(filter-out FORCE,$^) -o $@

export CPPFLAGS_vdso.lds += -P -C -Ush

vdso-flags = -shared -s -Wl,-soname=linux-vdso.so.1 \
		$(call cc-ldoption, -Wl$(comma)--hash-style=sysv)

CFLAGS_vnumber_of_the_beast.o = -fPIC
SYSCFLAGS_vdso.so = $(vdso-flags)

$(obj)/vdso.so: $(src)/vdso.lds $(foreach F, $(vobjs-y), $(obj)/$F) FORCE
	$(call if_changed,syscall)

