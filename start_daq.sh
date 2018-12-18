#!/bin/sh

#. setup.sh

./kill_daq.sh

mserver -D 

echo start obdedit
odbedit -c clean
#odbedit -s 100000000
#odbedit -c "rm /Analyzer/Trigger/Statistics"
#odbedit -c "rm /Analyzer/Scaler/Statistics"



echo start mhttpd
#mhttpd -p 8081 -D
sleep 2

echo start frontend and analyzer in terminals
#xterm -e ./analyzer &
echo start mlogger daemon
mhttpd  -D
#xterm -e ./frontend &
mlogger -D


echo Please point your web browser to http://localhost:8081
echo Or run: mozilla http://localhost:8081 &
echo To look at live histograms, run: roody -Hlocalhost
echo " "

#end file
