#!/bin/bash
if [ "$1" = "" ]; then
  echo usage: test.sh host:port
  exit 1
fi
host=http://$1
for (( i = 1 ; i <= 10 ; i = i + 1 )) ; do
  echo loop $i
  wget -r -l 1 -O /dev/null -o /dev/null $host
  sleep 1
  wget -O /dev/null -o /dev/null --post-data="fn=open&dev=/dev/cu.SLAB_USBtoUART" $1'/devpost.html?dev=/dev/cu.SLAV_USBtoUART&fn=open'
  wget -O /dev/null -o /dev/null $host
  j=20
  while [ $j -gt 0 ]; do
    wget -O /tmp/poll.xml -o /dev/null $1'/poll.xml'
    if ( test -s /tmp/poll.xml && grep 'log size="0"' /tmp/poll.xml > /dev/null 2>&1 ) ; then
      let j--
    else
      j=100
    fi
  done
  wget -O /dev/null -o /dev/null --post-data="fn=close&dev=/dev/cu.SLAB_USBtoUART" $1'/devpost.html?dev=/dev/cu.SLAB_USBtoUART&fn=close'
  wget -O /dev/null -o /dev/null $host
done
wget -O /dev/null -o /dev/null --post-data="fn=exit" $1'/devpost.html?dev=&fn=exit'
