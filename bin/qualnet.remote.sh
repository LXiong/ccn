#!/bin/sh

THIS_COMMAND=${0}
CONFIG_FILE=${1}
INTERACTIVE_PORT=${2}
CONFIG_DIR=${3}
NUM_PROCESSORS=${4}
LOGIN_AT_HOST=${5}
HOST_FILE=${6}
REMOTE_PRODUCT_HOME=${7}
REMOTE_RUN_MODE=${8}
PRODUCT_HOME_BIN=${9}
SIMULATION_MODE=${10}
STAT_FILE_NAME=${11}
GUI_MODE=${12}

if [ "${GUI_MODE}" = "" ]; then
	GUI_MODE="interactive"
fi

PRODUCT_HOME_KEY="QUALNET_HOME"
PRODUCT_HOME_VALUE=$QUALNET_HOME
PRODUCT_NAME="QualNet"
PRODUCT_REMOTE_DIR=".remoteQualnetDir"
PRODUCT_BINARY="qualnet"


message_first_time()
{
	echo
	echo "If this is the first time you are connecting to this server, then you must"
	echo "execute the following command on the client before running the simulation:"
	echo "  ${PRODUCT_HOME_VALUE}/bin/setupRemoteExec.sh  ${LOGIN_AT_HOST}"
}


error_missing_commands()
{
	echo
	echo " --------------------------------------------------------------------------------- "
	echo " ERROR: You must have the following installed and in your PATH "
	echo "        to execute ${PRODUCT_NAME} remotely: "
	echo ""
	echo "        scp "
	echo "        ssh "
	echo " --------------------------------------------------------------------------------- "
	message_first_time
	echo
	exit 1
}


if [ -z `type -p ssh` ] ; then  
	error_missing_commands
else
    if [ -z `type -p scp` ] ; then  
	    error_missing_commands
    fi
fi


echo "Executing $PRODUCT_NAME via remote ssh on host: ${LOGIN_AT_HOST}"
message_first_time

REMOTE_HOME_DIR=` ssh ${LOGIN_AT_HOST} pwd `
REMOTE_WORKING_DIR="${REMOTE_HOME_DIR}/${PRODUCT_REMOTE_DIR}"

INTERACTIVE_OPTION="-interactive localhost ${INTERACTIVE_PORT}"
PORT_FORWARDING_OPTION="-R"
if [ "${GUI_MODE}" = "player" ]; then
	INTERACTIVE_OPTION="-with_ocula_gui ${INTERACTIVE_PORT}"
	# Needs to be -L because sops/vops has the client/server ends for GUI/SIM reversed from interactive mode
	PORT_FORWARDING_OPTION="-L"
fi

REMOTE_SIM_COMMAND="\"${REMOTE_PRODUCT_HOME}/bin/${PRODUCT_BINARY}\" \"${CONFIG_FILE}\" -np ${NUM_PROCESSORS} ${INTERACTIVE_OPTION} ${SIMULATION_MODE} ${STAT_FILE_NAME}"
if [ "${REMOTE_RUN_MODE}" = "Distributed" ]; then
	REMOTE_SIM_COMMAND="mpirun -np ${NUM_PROCESSORS} -hostfile \"${HOST_FILE}\" \"${REMOTE_PRODUCT_HOME}/bin/${PRODUCT_BINARY}.mpi\" \"${CONFIG_FILE}\" ${INTERACTIVE_OPTION} ${SIMULATION_MODE}"
fi


echo
echo "Configuration File: ${CONFIG_FILE}"
echo "Number of Processors: ${NUM_PROCESSORS}"
echo "Remote ${PRODUCT_HOME_KEY}: ${REMOTE_PRODUCT_HOME}"
echo "Local Working Directory: ${CONFIG_DIR}"
echo "Remote Working Directory: ${REMOTE_WORKING_DIR}"
echo "Interactive Port: ${INTERACTIVE_PORT}"
echo "Remote Run Mode: ${REMOTE_RUN_MODE}"
echo "Simulation Mode: ${SIMULATION_MODE}"
echo "GUI Mode: ${GUI_MODE}"
echo "Remote Simulator Command: ${REMOTE_SIM_COMMAND}"
echo


ssh ${LOGIN_AT_HOST} "rm -rf ${REMOTE_WORKING_DIR}; mkdir ${REMOTE_WORKING_DIR}"

echo "Sending files to host"
cd "${CONFIG_DIR}"
scp -r * "${LOGIN_AT_HOST}:${REMOTE_WORKING_DIR}"
cd -
echo

ssh ${LOGIN_AT_HOST} "chmod -R 700 ${REMOTE_WORKING_DIR}"

echo "Starting ${PRODUCT_NAME} simulation"
ssh ${PORT_FORWARDING_OPTION} ${INTERACTIVE_PORT}:localhost:${INTERACTIVE_PORT} ${LOGIN_AT_HOST} \
"${PRODUCT_HOME_KEY}=\"${REMOTE_PRODUCT_HOME}\"; export ${PRODUCT_HOME_KEY}; cd ${REMOTE_WORKING_DIR}; ${REMOTE_SIM_COMMAND}"
echo "${PRODUCT_NAME} simulation complete"
echo


echo "Retrieving files from host"
scp -r ${LOGIN_AT_HOST}:${REMOTE_WORKING_DIR}/* "${CONFIG_DIR}"
echo

echo "Done"