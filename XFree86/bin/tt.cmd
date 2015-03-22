@echo off
echo Starting xfstt as local tcp daemon not using inetd...
(detach xfstt --port 7103 --once >/dev/console$ 2>&1) && echo Message log to /dev/console$. Now listening on port 7103...||echo Error! Look into /dev/console$ for messages...

echo Adding fontpath tcp/127.0.0.1:7103.... Please verify you put:
echo ##########################################################
echo ffst            7103/tcp           # TrueType X fontserver
echo ##########################################################
echo into %etc%/services!

(xset fp+ tcp/127.0.0.1:7103) && (echo Adding was successful. Stop with 'xset fp- tcp/127.0.0.1:7103'.)
