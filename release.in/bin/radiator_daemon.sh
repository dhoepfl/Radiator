#!  /bin/bash

PATH="$PATH:/home/radiator/opt/bin"

radiator \
	-o >(analyseStream.sh | mysql >/home/radiator/opt/var/log/radiator.log 2>&1) \
	/home/radiator/source/radiator/log.log.dd &

echo $! >/home/radiator/opt/var/run/radiator.pid
