#!/bin/sh

echo "Installing Cabbagelang..."
if [ `whoami` = "root" ];then
    echo "Root user detected."
else
    echo "You have to run this script as root, installation aborted."
    exit
fi

if test -e cabbage; then
    mkdir /lib/cabbagelang
    mkdir /lib/cabbagelang/leaves
    cp cabbage /bin/cabbage
    cp cabbage /lib/cabbagelang/cabbage
    echo "export CABBAGELANG_HOME=/lib/cabbagelang/" >> /etc/profile
else
    echo "cabbage does not exist, installation aborted."
    exit
fi

echo "Installed successfully."
