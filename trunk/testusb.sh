#!/bin/bash
for (( i = 1 ; i <= 10 ; i = i + 1 )) ; do
  echo loop $i
  wget -r -l 1 -O /dev/null -o /dev/null 'http://backup:9900'
  sleep 1
  echo open
  wget -O /dev/null -o /dev/null --post-data="fn=open&usb=true" 'http://backup:9900/devpost.html?dev=&fn=open&usb=true'
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
  echo close
  wget -O /dev/null -o /dev/null --post-data="fn=close&usb=true" 'http://backup:9900/devpost.html?dev=&fn=close&usb=true'
  wget -O /dev/null -o /dev/null 'http://backup:9900'
done
echo exit
wget -O /dev/null -o /dev/null --post-data="fn=exit" 'http://backup:9900/devpost.html?dev=&fn=exit'
