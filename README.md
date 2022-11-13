# MQTT
MQTT by sim800 series
There is a TLS version firmware for sim800 series module that support MQTT AT commands; But only support QoS 0,1 and text messages.
I wrote some functions that make you enable to publish hex buty array and QoS=2.
Thanks "http://www.raviyp.com/mqtt-protocol-tutorial-using-sim900-sim800-modules-mqtt-over-tcp/" and "http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html"
NOTE: some function used in this code like TCP_UDP_Send()that is a simply function (e.g:AT+CIPSEND=10), so I didn't define them.
