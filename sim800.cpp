bool MQTT_TCP_Connect(bool clean_session ,u16 keep_alive)
{
	bool ret = false;
	mqtt_keep_alive=keep_alive;
	///
	if(!MQTT_TCP_PING())
	{
		mqtt_conn_packet *conectpck;
		u8 packet[100];//,*ptr=packet;
		conectpck=(mqtt_conn_packet*)&packet[0];	
		Connect(MQTT_Url,MQTT_Port);
		char password[32];
		sprintf(conectpck->pname,"MQTT");
		conectpck->plen=strlen(conectpck->pname);
		conectpck->conn=0x10;
		conectpck->lvl=0x04;
		u8 reset=0;
		conectpck->flg=*(connect_flags*)&reset;
		conectpck->flg.cleansession=clean_session;
		conectpck->flg.usernameflg=1;
		conectpck->flg.passwordflg=1;
		conectpck->keepalive=keep_alive;//sec
		sprintf(conectpck->cid,"%s",MQTT_ClientID);
		conectpck->cidlen=strlen(conectpck->cid);
		sprintf(conectpck->user,"%s",MQTT_Usernameptr);
		conectpck->ulen=strlen(conectpck->user);
		sprintf(password,"12345678901");
		conectpck->pwlen=strlen(password);
		sprintf(conectpck->pw,"%s",password);
		conectpck->len=conectpck->plen+conectpck->cidlen+conectpck->ulen+conectpck->pwlen+2+1+1+2+2+2+2;
		//(x << 8) | (x >> 8)
		conectpck->plen=(conectpck->plen << 8) | (conectpck->plen >> 8);//big endian
		conectpck->cidlen=(conectpck->cidlen << 8) | (conectpck->cidlen >> 8);
		conectpck->ulen=(conectpck->ulen << 8) | (conectpck->ulen >> 8);
		conectpck->pwlen=(conectpck->pwlen << 8) | (conectpck->pwlen >> 8);
		conectpck->keepalive=(conectpck->keepalive << 8) | (conectpck->keepalive >> 8);
		Connecting_via_TCP=true;
		CheckDataConnection();//query connection status : AT+CIPSTATUS
		u8 to=10;
		while(!flag.TCP_connection && to)// wait to connect ok
		{
			CheckDataConnection();
			to--;
			delay_ms(1000);
		}
		if(to)
		{
			datareceived=false;
			if(!TCP_UDP_Send(packet,conectpck->len+2))return false;// my send function: AT+CIPSEND= 'msg len'\r\n
			to=100;
			while(to && !datareceived)//wait for ack : +IPD...
			{
				to--;
				delay_ms(100);
			}
			if(to)
			{
				u8 data[4];
				u16 rlen;
				ReadTcpData(data, sizeof(data),rlen);
				if(data[0]==MQTT_CONNACK<<4 && data[3]==MQTT_SUCCESS)
					ret=true;
				if(ret)
				{
					if(!clean_session)
					{
						delay_ms(200);
						MQTT_TCP_Poll();
					}
				}
			}
		}
	}
	else ret =true;
	
	return (ret);
	
}
u8 Len_encod(u16 len, mqtt_publish_Header* hlen)
{
	u8 out_len;
	u8 i=0;
	do
	{
		out_len =len % 128;
		len = len / 128;
		// if there are more data to encode, set the top bit of this byte
		if ( len > 0 )
			out_len |= 0x80;
		hlen->len[i++]=out_len;
	}while ( len > 0 );
	return i;
}
u16 SIM868_class:: Len_decod(u8* data)
{
	u32 multiplier = 1;
	u16 value = 0;
	u8 encodedByte;
	do
	{
		encodedByte = *data++;	
		value += (encodedByte & 127) * multiplier;
		multiplier *= 128;		
		if (multiplier > (128*128*128))
			return 0;
	}while ((encodedByte & 128) != 0);
	return value;
}
bool  MQTT_TCP_Publish(const char* topic,u8* message, u16 msglen, MQTT_QoS Qos, bool retain, bool dublicate)
{
	bool ret=false;
	mqtt_publish_Header* publishheader;
	mqtt_publich_payload* publich_pl;
		#if(msglen <450)
		u8 packet[500];
	#else 
		u8 packet[1500];
	#endif
	publishheader=(mqtt_publish_Header*)&packet[0];
	u8 topic_len=strlen(topic);	
	u8 len_field;
	u16 plen;
	if(Qos!=QoS_0)
	{
		len_field=Len_encod(msglen+topic_len+4,publishheader);
		plen=msglen+topic_len+4+1+len_field;
	}
	else
	{
		len_field=Len_encod(msglen+topic_len+2,publishheader);
		plen=msglen+topic_len+2+1+len_field;
	}
	publishheader=(mqtt_publish_Header*)&packet[0];
	publich_pl=(mqtt_publich_payload*)&packet[len_field+1];
	publishheader->cntflag.CNTPacket=MQTT_PUBLISH;
	publishheader->cntflag.Duplicate=dublicate;
	publishheader->cntflag.QoS=Qos;
	publishheader->cntflag.retain=retain;
	publich_pl->topic_len=topic_len;
	sprintf(publich_pl->topic,"%s",topic);
	if(Qos!=QoS_0){
		if(mqtt_pack_id==0)mqtt_pack_id=1;
		*(u16*)&packet[topic_len+2+1+len_field]=(mqtt_pack_id<< 8) | (mqtt_pack_id>>8);
		mqtt_pack_id++;
		memcpy(&packet[topic_len+5+len_field],message,msglen);
	}else
	{
		memcpy(&packet[topic_len+3+len_field],message,msglen);
	}
	publich_pl->topic_len=(publich_pl->topic_len << 8) | (publich_pl->topic_len >> 8);//big endian
	datareceived=false;
	
	if(!TCP_UDP_Send(packet,plen))return false;
	if(Qos!=QoS_0)
	{
		u8 to=150;
		while(to && !datareceived )
		{
			to--;
			delay_ms(100);
		}
		if(to)
		{
			datareceived=false;
			u8 data[4];
			u16 rlen;
			ReadTcpData(data, sizeof(data),rlen);
			u8 repAck=0;
			if(Qos==QoS_1) repAck=MQTT_PUBACK<<4;
			else
				repAck=MQTT_PUBREC<<4;
			if(data[0]==repAck)
			{
				if (Qos==QoS_2)
				{
					u16 id=*(u16*)&data[2];
					Pubrel(id);
					to=150;
					while(to && !datareceived )
					{
						to--;
						delay_ms(100);
					}
					if(to)
					{
						repAck=MQTT_PUBCOMP<<4;
						ReadTcpData(data, sizeof(data),rlen);
						if(data[0]==repAck)
							ret=true;
					}
				}
				else ret=true;
			}
				
		}
	}
	return (ret);
}
bool MQTT_TCP_Subscribe(const char* topic,MQTT_QoS Qos)
{
	bool ret =false;
	mqtt_subscribe_packet* subpacket;
	u8 topic_len=strlen(topic);
	u8 packet[150];
	subpacket=(mqtt_subscribe_packet*)&packet[0];
	subpacket->cntflag=MQTT_SUBSCRIBE<<4|0x02;//0x02 is reserved value
	subpacket->TopicLen=topic_len;
	if(mqtt_pack_id==0)mqtt_pack_id=1;
	subpacket->packID = mqtt_pack_id++;
	subpacket->packID=(subpacket->packID << 8) | (subpacket->packID >> 8);//big endian
	sprintf(subpacket->topic,"%s",topic);
	subpacket->len=subpacket->TopicLen+1+2+2;
	subpacket->TopicLen=(subpacket->TopicLen << 8) | (subpacket->TopicLen >> 8);//big endian
	packet[(subpacket->len+2)-1]=Qos;
	datareceived=false;
	if(!TCP_UDP_Send(packet,subpacket->len+2))return false;
	if(Qos!=0)
	{
		u8 to=100;
		while(to && !datareceived)
		{
			to--;
			delay_ms(100);
		}
		if(to)
		{
			u8 data[5];
			u16 rlen;
			datareceived=false;
			ReadTcpData(data, sizeof(data),rlen);
			if(data[0]==MQTT_SUBACK<<4 && data[4]==Qos)
				ret=true;
			/*Allowed return codes:
			0x00 - Success - Maximum QoS 0 
			0x01 - Success - Maximum QoS 1 
			0x02 - Success - Maximum QoS 2 
			0x80 - Failure */
		}
	}
	return (ret);
}
bool MQTT_TCP_UnSubscribe(const char* topic)
{
	bool ret =false;
	mqtt_subscribe_packet *unsub;
	u8 packet[150];
	unsub=(mqtt_subscribe_packet*)&packet[0];
	unsub->cntflag=MQTT_UNSUBSCRIBE<<4|0x02;//0x02 is reserved value
	if(mqtt_pack_id==0)mqtt_pack_id=1;
	unsub->packID = mqtt_pack_id++;
	unsub->packID=(unsub->packID << 8) | (unsub->packID >> 8);//big endian
	unsub->TopicLen=strlen(topic);
	unsub->len=unsub->TopicLen+2+2;
	unsub->TopicLen=(unsub->TopicLen << 8) | (unsub->TopicLen >> 8);
	datareceived=false;
	if(!TCP_UDP_Send(packet,unsub->len+2))return false;
	u8 to=100;
	while(to && !datareceived)
	{
		to--;
		delay_ms(100);
	}
	if(to)
	{
		u8 data[4];
		u16 rlen;
		datareceived=false;
		ReadTcpData(data, sizeof(data),rlen);
		if(data[0]==MQTT_UNSUBACK<<4 )
			ret=true;
	}
	return (ret);
}
bool MQTT_TCP_Disconnect(void)
{
	if(Query_Current_Connection()== Connection_Status_CLOSED)return true;
	bool ret=false;
	u8 disconn_packet[]={0xE0,0x00};
	if(!TCP_UDP_Send(disconn_packet,sizeof(disconn_packet)))return false;
	u8 to=10;
	while(to)
	{
		to--;
		delay_ms(1000);
		if(Query_Current_Connection()== Connection_Status_CLOSED)break;
	}
	if(to)ret= true;
	return ret;
}
bool MQTT_TCP_PING(void)
{
	if(!Connecting_via_TCP)//false when rted reset
		Connecting_via_TCP=true;
	CheckDataConnection();
	if(Query_Current_Connection()== Connection_Status_CLOSED)return false;
	bool ret =false;
	u8 ping_packet[]={0xC0,0x00};
	for(u8 i=0;i<3;i++)
	{
		datareceived=false;
		if(!TCP_UDP_Send(ping_packet,sizeof(ping_packet)))return false;
		u8 to=100;
		while(to && !datareceived)
		{
			to--;
			delay_ms(100);
		}
		if(to)
		{
			u8 data[4];
			u16 rlen;
			datareceived=false;
			ReadTcpData(data, sizeof(data),rlen);
			if(data[0]==MQTT_PINGRESP<<4)
				ret=true;
		}
		if(ret)break;
	}
	return (ret);
}
void MQTT_TCP_Poll(void)
{	
	if(datareceived)//datareceived set by interrupt
	{
		datareceived=false;
		u8 data[200];
		u16 rlen;
		ReadTcpData(data, sizeof(data),rlen);
		switch(data[0]& 0xF0)
		{
			case(MQTT_PUBLISH<<4):
			{
				MQTT_timout=Timer_Delay_GetTick() + (1000*(MQTT_login_timeout))*(1000/Timer_Delay_EachTickTime);
					
				mqtt_publish_Header* publishheader;
				publishheader=(mqtt_publish_Header*)&data[0];
				mqtt_publich_payload* packet;
				u16 plen=Len_decod(&data[1]);
				u8 len_field;
				if(plen>127)//farz bar ine ke len nahayat 2 byte
				{
					packet=(mqtt_publich_payload*)&data[3];
					len_field=2;
				}
				else
				{
					packet=(mqtt_publich_payload*)&data[2];
					len_field=1;
				}
				packet->topic_len=(packet->topic_len << 8) | (packet->topic_len >> 8);
				u16 id=data[packet->topic_len+4]<<8 | data[packet->topic_len+4+1];
				datareceived=false;
				if(publishheader->cntflag.QoS==QoS_1)
					PubAck(id);
				else if(publishheader->cntflag.QoS==QoS_2)
				{		
					Pubrec(id);
					u8 to =100;
					while(to && !datareceived){to--;delay_ms(100);}
					if(to)
					{
						datareceived=false;
						Pubcomp(id);
					}
				}
				u16 header_len=packet->topic_len+2+len_field+1;
				u16 msg_len=plen-header_len;
				//now process your data 
			}
			break;
			case(MQTT_DISCONNECT<<4):
			{
				Disconnect();
			}
			break;
		}
	}
	if(mqtt_keep_alive )
	{
		if(MQTT_Timer< Timer_Delay_GetTick())
		{
			MQTT_Timer=Timer_Delay_GetTick() + (1000*(mqtt_keep_alive))*(1000/Timer_Delay_EachTickTime);
			MQTT_TCP_Connect();
		}
	}
	
}
