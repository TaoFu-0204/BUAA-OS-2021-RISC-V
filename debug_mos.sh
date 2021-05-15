qemu-system-riscv64 -machine virt -nographic -bios default -device loader,file=gxemul/vmlinux,addr=0x80200000 -S -s
