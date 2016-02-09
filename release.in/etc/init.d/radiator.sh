#! /bin/sh
### BEGIN INIT INFO
# Provides:          radiator
# Required-Start:    mysql
# Required-Stop:
# Should-Start:      
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Starts the radiator monitor
# Description:       Read the radiator health data and log them to database
### END INIT INFO

PATH=/sbin:/bin:/usr/bin

. /lib/init/vars.sh
. /lib/lsb/init-functions

PID_FILE=/home/radiator/opt/var/run/radiator.pid
RADIATOR_BINARY=/home/radiator/opt/bin/radiator_daemon.sh

do_start () {
	log_daemon_msg "Starting Radiator health monitor" "Radiator"
	if pidofproc -p "$PID_FILE" "$RADIATOR_BINARY" >/dev/null ; then
		log_progress_msg "already running"
	else
		start_daemon -p "$PID_FILE" "$RADIATOR_BINARY"
		if [ "$?" -eq 0 ] ; then
			log_progress_msg "."
		else
			log_failure_msg "failed"
		fi
	fi
	log_end_msg 0

	exit 0
}

do_stop () {
	log_daemon_msg "Stopping Radiator health monitor" "Radiator"
	if pidofproc -p "$PID_FILE" "$RADIATOR_BINARY" >/dev/null ; then
		killproc -p "$PID_FILE" "$RADIATOR_BINARY"
	else
		log_progress_msg "not running"
	fi
	log_end_msg 0

	exit 0
}

do_status () {
	log_daemon_msg "Radiator health monitor" "Radiator"
	if pidofproc -p "$PID_FILE" "$RADIATOR_BINARY" >/dev/null ; then
		log_progress_msg "running"
	else
		log_progress_msg "not running"
	fi
	log_end_msg 0

	exit 0
}

case "$1" in
  start)
	do_start
	;;
  restart|reload|force-reload)
	echo "Error: argument '$1' not supported" >&2
	exit 3
	;;
  stop)
	do_stop
	;;
  status|"")
	do_status
	exit $?
	;;
  *)
	echo "Usage: radiator.sh [start|stop]" >&2
	exit 3
	;;
esac

:
