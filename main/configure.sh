#!/bin/bash


install_dir=".."
base_exe_name=qualnet

while getopts "d:e:hpv" opt
do
	case "$opt" in
		d) install_dir=$OPTARG;;
		e) base_exe_name=$OPTARG;;
		p) print_only=1; verbose=1;;
		v) verbose=1;;
		h) echo "$0 
            -d        Installation Directory (default: ".")
            -e        Executable name (default: qualnet)
			-v        Verbose output
            -h        Print this help message"
			exit;;
	esac
done

debug_print() {
	MSG=$1
	if [ $verbose ]; then
		echo $MSG
	fi
	echo $MSG >> /tmp/installer-log-$USER
}

>/tmp/installer-log-$USER
debug_print "Starting Installation-Directory=$install_dir Execution-Name=$base_exe_name"

proc_type=`uname -m`
if [ $proc_type == "" ]
then
        debug_print "Cannot identify the type (32-bit or 64-bit) of processor on this machine"
        exit 2
fi

OS32or64=`uname -m`

if [ "$OS32or64" == "i686" ]; then
for f in /lib32/libc-*.so /lib/i386-linux-gnu/libc-*.so /lib/libc-*.so; do if [ -f $f ]; then libc_file=$f; break; fi; done
glibc_version=`"$libc_file" | head -n 1 | sed -e 's/.* version \([0-9.]\+\).*/\1/'`
fi

if [ "$OS32or64" == "x86_64" ]; then
for f in /lib64/libc-*.so /lib/x86_64-linux-gnu/libc-*.so /lib/libc-*.so; do if [ -f $f ]; then libc_file=$f; break; fi; done
glibc_version=`"$libc_file" | head -n 1 | sed -e 's/.* version \([0-9.]\+\).*/\1/'`
fi
debug_print "GLIBC version is ${glibc_version:n:4}" 

## Identify the gcc version based on gcc version
if [ ${glibc_version:n:4} == "2.3" ]; then
	gcc_version="4.0"
fi
if [ ${glibc_version:n:4} == "2.4" ]; then
	gcc_version="4.1"
fi
if [ ${glibc_version:n:4} == "2.5" ]; then
	gcc_version="4.1"
fi
if [ ${glibc_version:n:4} == "2.8" ]; then
	gcc_version="4.3"
fi
if [ ${glibc_version:n:4} == "2.9" ]; then
	gcc_version="4.3"
fi
if [ ${glibc_version:n:4} == "2.10" ]; then
	gcc_version="4.4"
fi
if [ ${glibc_version:n:4} == "2.15" ]; then
	gcc_version="4.6"
fi

debug_print "GCC version matched for glibc version ${glibc_version:n:4} is $gcc_version" 

proc_type=`uname -m`
if [ $proc_type == "" ]
then
        debug_print "Cannot identify the type (32-bit or 64-bit) of processor on this machine"
        exit 2
fi

if [ $proc_type == "x86_64" ]
then
        proc_type="linux-x86_64"
        tag_name="64bit"
else
        proc_type="linux"
        tag_name="32bit"
fi

debug_print "Platform type is $proc_type" 


# REMOVED # we always install the 32-bit binary
installer_proc_type=$proc_type

# Identify the correct makefile and executables
makefile=Makefile-$proc_type-glibc-${glibc_version:n:4}-gcc-$gcc_version
exe_name=$base_exe_name-$proc_type-glibc-${glibc_version:n:4}-gcc-$gcc_version
radio_range_name=radio_range-$proc_type-glibc-${glibc_version:n:4}-gcc-$gcc_version
shptoxml_name=shptoxml-$proc_type-glibc-${glibc_version:n:4}-gcc-$gcc_version
mts_socket_name=mts-socket-$proc_type-glibc-${glibc_version:n:4}-gcc-$gcc_version

# Check if the makefile and executable for this version exist in the distribution
if [ ! -f $install_dir/main/$makefile ]
then
	debug_print "Makefile [$install_dir/main/$makefile] does not exist" 
	exit 1
fi

debug_print "Found correct makefile!" 
debug_print "Copying $install_dir/main/$makefile to $install_dir/main/Makefile" 
# Copy the correct version of makefile and executable
if [ ! $print_only ]; then
	cp $install_dir/main/$makefile $install_dir/main/Makefile
fi

if [ ! -f $install_dir/bin/$exe_name ]
then
	debug_print "Executable [$install_dir/bin/$exe_name] does not exist" 
	exit 1
fi

debug_print "moving $install_dir/bin/$exe_name to $install_dir/bin/$base_exe_name" 
if [ ! $print_only ]; then
	mv $install_dir/bin/$exe_name $install_dir/bin/$base_exe_name
	cp $install_dir/bin/$base_exe_name $install_dir/bin/$base_exe_name-precompiled-$tag_name
	rm -f $install_dir/bin/$base_exe_name-linux-*
fi


if [ ! -f $install_dir/bin/$radio_range_name ]
then
	debug_print "Executable [$install_dir/bin/$radio_range_name] does not exist" 
	exit 1
fi

debug_print "moving $install_dir/bin/$radio_range_name to $install_dir/bin/$range" 
if [ ! $print_only ]; then
	mv $install_dir/bin/$radio_range_name $install_dir/bin/radio_range
	cp $install_dir/bin/radio_range $install_dir/bin/radio_range-precompiled-$tag_name
	rm -f $install_dir/bin/radio_range-linux-*
fi

if [ ! -f $install_dir/bin/$shptoxml_name ]
then
	debug_print "Executable [$install_dir/bin/$shptoxml_name] does not exist" 
	exit 1
fi

debug_print "moving $install_dir/bin/$shptoxml_name to $install_dir/bin/$range" 

if [ ! $print_only ]; then
	mv $install_dir/bin/$shptoxml_name $install_dir/bin/shptoxml
	cp $install_dir/bin/shptoxml $install_dir/bin/shptoxml-precompiled-$tag_name
	rm -f $install_dir/bin/shptoxml-linux-*
fi

if [ $print_only ]; then
	exit 0
fi

if [ ! -f $install_dir/bin/$mts_socket_name ]
then
	debug_print "Executable [$install_dir/bin/$mts_socket_name] does not exist" 
	exit 0
fi

debug_print "moving $install_dir/bin/$mts_socket_name to $install_dir/bin/$range"

if [ ! $print_only ]; then
	mv $install_dir/bin/$mts_socket_name $install_dir/bin/mts-socket
	cp $install_dir/bin/mts-socket $install_dir/bin/mts-socket-precompiled-$tag_name
	rm -f $install_dir/bin/mts-socket-linux-*
fi

if [ $print_only ]; then
        exit 0
fi
