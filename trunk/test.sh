#!/bin/bash
for (( i = 1 ; i <= 10 ; i = i + 1 )) ; do
  echo loop $i
  wget -r -l 1 -O /dev/null -o /dev/null 'http://backup:9900'
  sleep 1
  wget -O /dev/null -o /dev/null --post-data="open=" 'http://backup:9900/devpost.html?dev=/dev/ttyUSB1&fn=open'
  wget -O /dev/null -o /dev/null 'http://backup:9900'
  j=20
  while [ $j -gt 0 ]; do
    wget -O /tmp/poll.xml -o /dev/null 'http://backup:9900/poll.xml'
    if ( test -s /tmp/poll.xml && grep 'log size="0"' /tmp/poll.xml > /dev/null 2>&1 ) ; then
      let j--
    else
      j=100
    fi
  done
  wget -O /dev/null -o /dev/null --post-data="close=" 'http://backup:9900/devpost.html?dev=/dev/ttyUSB1&fn=close'
  wget -O /dev/null -o /dev/null 'http://backup:9900'
done
wget -O /dev/null -o /dev/null --post-data="exit=" 'http://backup:9900/devpost.html?dev=&fn=exit'
