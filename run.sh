make
CFILE="test.c"
NAME="${CFILE%.c}"
KOFILE="${NAME}.ko"
sudo insmod $KOFILE
sudo dmesg | tail -n 10
sudo lsmod | grep $NAME
sudo rmmod $NAME
make clean
