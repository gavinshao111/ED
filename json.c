{"ServiceType":"viedoPlayer","Data_Type":"Realtime","URL":"rtsp://120.27.188.84:8888/realtime/$1234/1/realtime.sdp","VideoType":"SD","Operation":"Begin","Datetime":"1480735266671"}

{"ServiceType":"viedoPlayer","Data_Type":"Realtime","URL":"rtsp://120.27.188.84:8888/realtime/$1234/1/realtime.sdp","VideoType":"","Operation":"Stop","Datetime":"1480735281212"}

EasyDarwin与车机MQTT的实时视频通讯JSON发送

{

"ServiceType":"viedoPlayer",           //发送视频信息  need when stop

"Data_Type":" Realtime",		// need when stop

"URL":"rtsp://ip:Port/realtime/$clientid/realtime.sdp",   // need when stop

"VideoType":"SD",                              //HD高清        SD标清

"Operation":" Begin"  / "Stop",

"Datetime":"1480474086800"

}  

EasyDarwin与车机MQTT的录像通讯JSON发送

{

"ServiceType":"viedoPlayer",           //发送视频信息

"Data_Type":" Recording",

"URL":"rtsp://ip:Port/record/$clientid/20140820163420.sdp",

"VideoType":"SD",                              //HD高清        SD标清

"CurrentTime":"0",                             //默认单位为秒，录像拖动时需要

"Operation":" Begin",

"Datetime":"1480474086800"

}  

发送JSON的Topic:" /clientID/videoinfoAsk" 
/1234/videoinfoAsk
/carleapmotorCLOUDE20160727inform/videoinfoAsk
realtime url:
rtsp://120.27.188.84:8888/realtime/$1234/1/realtime.sdp
rtsp://120.27.188.84:8888/realtime/$carleapmotorCLOUDE20160727inform/1/realtime.sdp

record
rtsp://120.27.188.84:8888/record/$carleapmotorCLOUDE20160727inform/1/2016-08-30_113613.sdp 


test:
rtsp://120.27.188.84:8888/realtime/$1234/1/realtime.sdp
rtsp://192.168.61.128:8888/realtime/$carleapmotorCLOUDE20160727inform/1/realtime.sdp

play record in EasyDarwin:
rtsp://120.27.188.84:8888/testAfterHint.mp4
rtsp://120.27.188.84:8888/record/$phoneapptest/1/123.sdp