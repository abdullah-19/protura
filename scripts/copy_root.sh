#!/bin/bash -e
#
# Copies the contents of the `./disk/*` folders into a new root system, along
# with creating extra required folders and such. It also correctly sets up the
# user ids on the file system.
#
# Argument 1: The location of current copy of root
# Argument 2: The location of the kernel binaries to put in /boot
# Argument 3: The location of extra files to copy into root, along with device_table.txt
# Argument 4: The location to copy root files into
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

OLD_ROOT=$1
KERNEL_BINS=$2
EXTRA_ROOT_DIR=$3
NEW_ROOT=$4

cp -R $OLD_ROOT/* $NEW_ROOT/

mkdir $NEW_ROOT/mnt
mkdir $NEW_ROOT/mnt/slave

cp -R $EXTRA_ROOT_DIR/home $NEW_ROOT/home

mkdir $NEW_ROOT/root
touch $NEW_ROOT/root/root_file

# Only root can view /root
chown -R 0:0 $NEW_ROOT/root
chmod 760 -R $NEW_ROOT/root

# su needs to be suid to work correctly
chmod +s $NEW_ROOT/bin/su

# apply proper ownership to home directories
chown -R 1000:1000 $NEW_ROOT/home/mkilgore/
chown -R 1001:1001 $NEW_ROOT/home/exuser/

mkdir $NEW_ROOT/etc
cp -R $EXTRA_ROOT_DIR/etc/* $NEW_ROOT/etc/

# Apply http ownership to 'server' directory
mkdir $NEW_ROOT/srv
mkdir $NEW_ROOT/srv/http
chown -R 33:33 $NEW_ROOT/srv

mkdir $NEW_ROOT/tests
cp -R $EXTRA_ROOT_DIR/tests/* $NEW_ROOT/tests

mkdir $NEW_ROOT/proc
mkdir $NEW_ROOT/tmp
mkdir $NEW_ROOT/dev

mkdir $NEW_ROOT/var
mkdir $NEW_ROOT/var/log

mkdir $NEW_ROOT/boot
mkdir $NEW_ROOT/boot/grub

cp -R $KERNEL_BINS/* $NEW_ROOT/boot
cp -R $DIR/grub.cfg $NEW_ROOT/boot/grub

# Apply global permissions and sticky bit to /tmp
chmod 1777 $NEW_ROOT/tmp

# Create /dev nodes based on device_table.txt file
while read device; do
    if [ ! -z "$device" ]; then
        mknod $NEW_ROOT/$device
        chmod 666 $NEW_ROOT/$(echo "$device" | cut -d' ' -f1)
    fi
done < $EXTRA_ROOT_DIR/device_table.txt

