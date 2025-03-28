#!/bin/sh 

mdisk=`nvram get k3c_disk`
usbmount=`ls /tmp/mnt/`

stop() {
killall -9 ngrokc

}

start() {
#开机启动太早会卡？
if [ "$usbmount" == "" ];then
    echo " $(date "+%F %T"):""系统正在启动，等待USB设备挂载中！" >> /tmp/frpc.log
fi
while [ "$usbmount" == "" ]
do
	sleep 5s
	usbmount=`ls /tmp/mnt/ |grep $mdisk`
done
	usb_uuid=`dbus get jffs_ext`
	if [ -n "$usb_uuid" ]; then
		mdisk=`blkid |grep "${usb_uuid}" |cut -c6-9`
	else
		mdisk=`nvram get k3c_disk`
	fi
	usb_disk="/tmp/mnt/$mdisk"
#不重复启动
icount=`ps -w|grep ngrokc|grep -v grep|wc -l`

if [ $icount = 0  ] ;then

mserver=`nvram get ngrok_server`
mport=`nvram get ngrok_port`
mtoken=`nvram get ngrok_token`
mrulelist=`nvram get ngrok_rulelist`

while [ $icount -lt 32 ]  
do 

let mloc=icount+2

#mstr=`echo "$mrulelist"| awk -F'<' '{print \$$mloc}'
#mcmd="echo \"$mrulelist\"|awk -F '<' '{printf \$$mloc}'|sed \"s/>/ /g\""
mcmd="echo \"$mrulelist\"|awk -F '<' '{printf \$$mloc}'"

mstr=`echo $mcmd |sh`

mtype=`echo "$mstr"|awk -F'>' '{printf $1}'`
mhost=`echo "$mstr"|awk -F'>' '{printf $3}'`
mlport=`echo "$mstr"|awk -F'>' '{printf $4}'`
mname=`echo "$mstr"|awk -F'>' '{printf $2}'`
mrport=`echo "$mstr"|awk -F'>' '{printf $5}'`



if [ -z "$mstr"  ] ;then
exit 0
fi
if [ ! -e "/jffs/softcenter/bin/ngrokc" ]; then
wget --no-check-certificate --timeout=10 --tries=3 -qO /jffs/softcenter/bin/ngrokc http://k3c.paldier.com/tools/ngrokc
if [ "$?" == "0" ]; then
chmod 755 /jffs/softcenter/bin/ngrokc
logger -t "【ngrok】" "下载成功!"
fi
if [ "$mtype" = "tcp" ] ;then
/jffs/softcenter/bin/ngrokc -SER[Shost:$mserver,Sport:$mport,Atoken:$mtoken] -AddTun[Type:$mtype,Lhost:$mhost,Lport:$mlport,Rport:$mrport,Sdname:$mname] &
else
/jffs/softcenter/bin/ngrokc -SER[Shost:$mserver,Sport:$mport,Atoken:$mtoken] -AddTun[Type:$mtype,Lhost:$mhost,Lport:$mlport,Sdname:$mname] &
fi 

let icount=icount+1 

done

fi

}

restart() {
  stop
  sleep 1
  menable=`nvram get ngrok_enable`
  kenable=`nvram get k3c_enable`
  if [ "$menable" == "1" -a "$kenable" == "1" ] ;then
  start
  else if [ "$menable" == "1" ]
    logger -t "软件中心""jffs扩展挂载未开启！"
  fi
}

restart

