/*
 * INFORMATION ****************************************************
 */



#include "sender.h"




server_command_params_t unpackCommand(uint8_t* pkt)
{
    server_command_params_t command;
    command.cmd = pkt[0];
    
    switch(command.cmd)
    {
        
        case CMD_STATUS:
            memcpy(&command.timestamp,pkt+1,sizeof(unsigned long));
            memcpy(&command.ms,       pkt+5,sizeof(unsigned short));
            break;
        case CMD_STARTREC:
            memcpy(&command.timestamp,pkt+1,sizeof(unsigned long));     // get timestamp from packet
            memcpy(&command.ms,       pkt+5,sizeof(unsigned short));    // get milliseconds from packet
            memcpy(&command.timeout,  pkt+7,sizeof(unsigned short));    // get timeout from packet
            break;
        case CMD_ENDREC:
            break;
        case CMD_REQUNSENT:
            break;
        case CMD_REQSINCE:
            memcpy(&command.timestamp,pkt+1,sizeof(unsigned long));
            memcpy(&command.ms,       pkt+5,sizeof(unsigned short));
            break;
        default:
            command.cmd = CMD_INVALID;
            break;
    }
    return command;
}

void sender_init()
{
    dateReceived = false;
    pendingCommand.cmd = CMD_NONE;   
}

bool updateSender()
{
    if(pendingCommand.cmd != CMD_NONE)
    {
        server_command_params_t command;
        command = pendingCommand;
    
        switch(command.cmd)
        {
        
            case CMD_STATUS:
                debug_log("SENDER: Got CMD_STATUS - ts %lu, ms %hu\r\n",command.timestamp,command.ms);
                if(send.bufContents != SENDBUF_STATUS)
                {
                    send.buf[0] = (dateReceived) ? 1 : 0;
                    send.buf[1] = 1;  // UNSENT DATA READY
                    send.buf[2] = 1;  // COLLECTING DATA
                    
                    unsigned long timestamp = 0;
                    unsigned short ms = 0;
                    if(dateReceived)
                    {
                        timestamp = now();
                        ms = nowFractional();
                    }
                    memcpy(send.buf+3,&timestamp,sizeof(unsigned long));
                    memcpy(send.buf+7,&ms,sizeof(unsigned short));
                    
                    float voltage = getBatteryVoltage();
                    memcpy(send.buf+9,&voltage,sizeof(float));
                    
                    send.bufContents = SENDBUF_STATUS;
                    
                    setTimeFractional(command.timestamp,command.ms);
                }
                if(BLEwrite(send.buf,SENDBUF_STATUS_SIZE))
                {
                    send.bufContents = SENDBUF_EMPTY;   // buffer has been sent
                    pendingCommand.cmd = CMD_NONE;      // we're done with that pending command
                    debug_log("SENDER: Sent status.\r\n");
                }
                break;
            case CMD_STARTREC:
                debug_log("SENDER: Got CMD_STARTREC - ts %lu, ms %hu, to %hu\r\n",command.timestamp,command.ms,command.timeout);
                pendingCommand.cmd = CMD_NONE;
                break;
            case CMD_ENDREC:            // server requests stop collecting
                debug_log("SENDER: Got CMD_STATUS\r\n");
                pendingCommand.cmd = CMD_NONE;
                break;
            case CMD_REQUNSENT:         // server requests send unsent data from flash
                debug_log("SENDER: Got CMD_REQUNSENT\r\n");
                pendingCommand.cmd = CMD_NONE;
                break;
            case CMD_REQSINCE:
                debug_log("SENDER: Got CMD_REQSINCE - ts %lu, ms %hu\r\n",command.timestamp,command.ms);
                pendingCommand.cmd = CMD_NONE;
                break;
            default:
                debug_log("SENDER: ERR: Invalid server command.\r\n");
                pendingCommand.cmd = CMD_NONE;
                break;
        }
    }
    return false;
}


/*void BLEonReceive(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length) 
{
    if(length > 0)
    {
        pendingCommand = unpackCommand(p_data);
    }
}*/