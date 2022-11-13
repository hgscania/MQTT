bool MQTT_TCP_Connect(bool clean_session=0,u16 keep_alive=60);
bool MQTT_TCP_Publish(const char* topic,u8* message, u16 msglen, MQTT_QoS Qos=QoS_2, bool retain=0, bool dublicate=0);
bool MQTT_TCP_Subscribe(const char* topic,MQTT_QoS Qos=QoS_2);
bool MQTT_TCP_UnSubscribe(const char* topic);
bool MQTT_TCP_Disconnect(void);
void MQTT_TCP_Poll(void);
bool MQTT_TCP_PING(void);
bool PubAck(u16 id);
bool Pubrec(u16 id);
bool Pubrel(u16 id);
bool Pubcomp(u16 id);
u8 Len_encod(u16 len, mqtt_publish_Header* hlen);
u16 Len_decod(u8* data);
			typedef enum
			{
				QoS_0=0,
				QoS_1=1,
				QoS_2=2
			}MQTT_QoS;
			typedef enum
			{
				MQTT_CONNECT     = 1,
				MQTT_CONNACK     = 2,
				MQTT_PUBLISH     = 3,
				MQTT_PUBACK      = 4,
				MQTT_PUBREC      = 5,
				MQTT_PUBREL      = 6,
				MQTT_PUBCOMP     = 7,
				MQTT_SUBSCRIBE   = 8,
				MQTT_SUBACK      = 9,
				MQTT_UNSUBSCRIBE =10,
				MQTT_UNSUBACK    =11,
				MQTT_PINGREQ     =12,
				MQTT_PINGRESP    =13,
				MQTT_DISCONNECT  =14	
			}Mqtt_cnt_packet;
			typedef enum
			{
				MQTT_CONNECTION_REFUSED            =-2,
				MQTT_CONNECTION_TIMEOUT            =-1,
				MQTT_SUCCESS                       = 0,
				MQTT_UNACCEPTABLE_PROTOCOL_VERSION = 1,
				MQTT_IDENTIFIER_REJECTED           = 2,
				MQTT_SERVER_UNAVAILABLE            = 3,
				MQTT_BAD_USER_NAME_OR_PASSWORD     = 4,
				MQTT_NOT_AUTHORIZED                = 5
	
			}Mqtt_connect_err;

      #pragma pack(push)
			#pragma pack(1)
			typedef struct{
				u8 reserved:1;
				u8 cleansession:1;
				u8 willflg:1;
				u8 willqos:2;
				u8 willretain:1;
				u8 passwordflg:1;
				u8 usernameflg:1;
			}connect_flags;
			typedef struct{
				u8 conn;
				u8 len;
				u16 plen;
				char pname[4];
				u8 lvl;
				connect_flags flg;
				u16 keepalive;
				u16 cidlen;
				char cid[10];
				u16 ulen;
				char user[10];
				u16 pwlen;
				char pw[11];
			}mqtt_conn_packet;
			typedef struct
			{
				u8 retain:1;
				u8 QoS:2;
				u8 Duplicate:1;
				u8 CNTPacket:4;
			}pub_flag;
			typedef struct
			{
				pub_flag cntflag;
				u8 len[1];
			}mqtt_publish_Header;
			typedef struct
			{
				u16 topic_len;
				char topic[1];
			}mqtt_publich_payload;
			typedef struct{
				u8 cntflag;
				u8 len;
				u16 packID;
				u16 TopicLen;
				char topic[1];
				
			}mqtt_subscribe_packet;
			typedef struct{
				u8 reserved:4;
				u8 cntpacket:4;
				u8 remaininglen;
				u16 packetID;
			}mqtt_pub_ack;
			#pragma pack(pop)
