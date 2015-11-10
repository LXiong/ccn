#!/bin/sh

echo "QualNet SSH remote execution setup"


REMOTE_HOST=$1
if [ -z $REMOTE_HOST ]; then
	echo "Usage:  $0  username@host"
	exit
fi

echo "Configuring security settings to $REMOTE_HOST to require no password with this client"
echo



#create an rsa key if none exist
if [ ! -e ~/.ssh/id_rsa ]; then
	ssh-keygen -q -b 2048 -t rsa -N "" -C "" -f ~/.ssh/id_rsa
fi




MY_KEY=`cat ~/.ssh/id_rsa.pub`

#make ssh settings directory
REMOTE_COMMANDS="mkdir ~/.ssh; "

#append to authorized keys file(s)
REMOTE_COMMANDS="$REMOTE_COMMANDS echo \"$MY_KEY\" >> ~/.ssh/authorized_keys2; "
REMOTE_COMMANDS="$REMOTE_COMMANDS echo \"$MY_KEY\" >> ~/.ssh/authorized_keys; "

#set the correct permissions
REMOTE_COMMANDS="$REMOTE_COMMANDS chmod 600 ~/.ssh/authorized_keys2; "
REMOTE_COMMANDS="$REMOTE_COMMANDS chmod 600 ~/.ssh/authorized_keys "

#send the remote commands
ssh $REMOTE_HOST "$REMOTE_COMMANDS"

echo "Done"
