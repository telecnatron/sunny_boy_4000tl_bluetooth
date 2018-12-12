// -----------------------------------------------------------------------------
// Copyright Stephen Stebbing, telecnatron.com. 2014.
// $Id: $
// 
// Tool to read power production data for SMA solar power convertors 
//
// Derived from on orignal code by Wim Hofman,
// See http://www.on4akh.be/SMA-read.html
// 
// This code is released to the public domain.
// -----------------------------------------------------------------------------
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <errno.h>
#include <libgen.h> // for basename()
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "logger.h"
#include "crc.h"


// globals
//! These are the 'macro' string that may be contained in the sb.ini script file
char *accepted_strings[] = {
"$END",
"$ADDR",
"$TIME",
"$SER",
"$CRC",
"$POW",
"$DTOT",
"$ADD2",
"$CHAN"
};

// flag to indicate the output messages should be verbose
int verbose ;
// buffer for holding the characters read from XXX (script file?)
unsigned char fl[1024] ;
// index into the buffer of current character
int cc;
// timeout in seconds for reading from socket
uint8_t sb_sock_read_timeout_sec = 7;
// name of the script file
char scriptFNameDefault[]="/etc/sbread.script";
char *scriptFName = scriptFNameDefault;

// flag to indicate whether instantaneous power, energy so far today, or both should be displayed
#define DISPLAY_POWER     0
#define DISPLAY_ENERGY    1
#define DISPLAY_BOTH      2
uint8_t display_flag=DISPLAY_POWER;


//! calculate the crc  for current data
void tryfcs16(unsigned char *cp, int len)
{
    uint16_t trialfcs;
    int i;	 

    /* add on output */
    if (verbose ==1){
	printf("String to calculate FCS\n");	 
	for (i=0;i<len;i++) 
	    printf("%02X ",cp[i]);
	printf("\n\n");
    }	
    trialfcs = crc_calc_crc( CRC_PPPINITFCS16, cp, len );
    trialfcs ^= 0xffff;               /* complement */
    fl[cc] = (trialfcs & 0x00ff);    /* least significant byte first */
    fl[cc+1] = ((trialfcs >> 8) & 0x00ff);
    cc = cc + 2;
    if (verbose == 1) printf("FCS = %x%x\n",(trialfcs & 0x00ff),((trialfcs >> 8) & 0x00ff)); 
}

//! convert something or another
unsigned char conv(char *nn)
{
    unsigned char tt=0,res=0;
    int i;   
    
    for(i=0;i<2;i++){
	switch(nn[i]){
	    case 65:
		tt = 10;
		break;
	    case 66:
		tt = 11;
		break;
	    case 67:
		tt = 12;
		break;
	    case 68:
		tt = 13;
		break;
	    case 69:
		tt = 14;
		break;
	    case 70:
		tt = 15;
		break;
	    default:
		tt = nn[i] - 48;
	}
	res = res + (tt * pow(16,1-i));
    }
    return res;
}

int select_str(char *s)
{
    int i;
    for (i=0; i < sizeof(accepted_strings)/sizeof(*accepted_strings);i++)
	if (!strcmp(s, accepted_strings[i])) return i;
    return -1;
}

/** 
 * Display info on commandline parameters
 * @param exeName Pointer to string being name of the binary executable file being run, 
 * this would usually be argv[0]
 */
void usage(char* exePath)
{
    fprintf(stderr,"Usage: %s -address XX:XX:XX:XX:XX:XX -serial XX:XX:XX:XX [-script /path/to/script] [-v] [-vv] [-d] [-b]\nWhere:\n",basename(exePath));
    fprintf(stderr,"\t-address  specifies the bluetooth of the inverter. eg -address 00:80:25:A6:77:60\n");
    fprintf(stderr,"\t-serial   specifies the serial number of the inverter converted to hexidecimal. \n");
    fprintf(stderr,"\t	         eg: Inverter with s/n 2130248863 would be -serial 7E:F9:04:9F\n");
    fprintf(stderr,"\t-script   (optional) specifies the path to the script file.\n\t\t\t  If not present, default script file %s is used.\n", scriptFName);
    fprintf(stderr,"\t-d        display total energy produced so far today (in kWh), instead of current power\n");
    fprintf(stderr,"\t-b        display both current power and energy produced so far today. eg: 3077,13.40\n");
    fprintf(stderr,"\t-v        specifies that verbose messages will be displayed while the program runs.\n\t\t\t  Use -vv for very verbose (debug)messages.\n");
    fprintf(stderr,"\t-h        display this help message\n\n");
}

//! log the passed data, but only if logger_level is set to debug
void log_data_debug(char* prefix, unsigned char* data, unsigned len){
    if(logger_level == LOGGER_LEVEL_DEBUG) {
	logger_start(LOGGER_LEVEL_DEBUG);
	logger_output(prefix);
	for (int i=0;i<len;i++) 
	    logger_fmt_output("%02x ",data[i]);
	logger_end();
    }
}


int main(int argc, char **argv)
{
    LOGGER_SET_LEVEL(LOGGER_LEVEL_ERROR);

    // pointer to config file
    FILE *fp;

    // buffer for reading character from socket
    // XXX and possibly other stuff
    unsigned char buf[1024];
    // Received bytes unescaped and are stored here
    unsigned char received[1024];
    int i;
    int ret,found=0;

    //! bluetooth address of inverter as string. eg 00:80:25:A6:77:60
    char sbAddrStr[18]={'\x0'};
    //! serial number of inverter as hex string. eg sn: 2130248863 -> 7e:f9:04:9f
    char sbSerialStr[18]={'\x0'};
    
    char line[400];

    //! bluetooth address of the inverter in binary
    char sb_bt_addr[6] = { 0 };
    //! our bluetooth address in binary
    char our_bt_addr[6] = { 0 };
    //! inverter serial number - converted to sequence of hex bytes
    char serial[4] = { 0 };
   
    char *lineread;
    char tt[10] = {48,48,48,48,48,48,48,48,48,48}; 
    char ti[3];	
    char pac[2];
    char tot[2];

    // bluetooth channel as returned from the sb
    char chan[1];
    char rowres[10];
    int rr;

    // current power being produced
    int currentpower;
    // day's total energy produced
    float dtotal;

    // zero the buffer
    memset(buf,0,1024);
    // zero received buffer
    memset(received,0,1024);
    
    // process command line arguments
    for (i=1;i<argc;i++){
	// inverter bt address XX:XX:XX:XX:XX:XX
	if (strcmp(argv[i],"-address")==0){
	    i++;
	    if (i<argc){
		strcpy(sbAddrStr,argv[i]);
	    }else{
		usage(argv[0]);
		return(-1);
	    }
	}
	// inverter serial number as hex digits XX:XX:XX:XX
	if (strcmp(argv[i],"-serial")==0){
	    i++;
	    if (i<argc){
		strcpy(sbSerialStr,argv[i]);
	    }else{
		usage(argv[0]);
		return(-1);
	    }
	}
	// -v flags log level info
	if (strcmp(argv[i],"-v")==0){
	    LOGGER_SET_LEVEL(LOGGER_LEVEL_INFO);
	}
	// -vv flags log level debug
	if (strcmp(argv[i],"-vv")==0){
	    LOGGER_SET_LEVEL(LOGGER_LEVEL_DEBUG);
	}
	// script file
	if(strcmp(argv[i],"-script")==0){
	    i++;
	    if(i<argc){
		scriptFName=argv[i];
		LOGGER_FMT_DEBUG("using script file: %s",scriptFName);
	    }else{
		usage(argv[0]);
		return(-1);
	    }
	}
	// display energy produced so far today
	if (strcmp(argv[i],"-d")==0){
	    display_flag=DISPLAY_ENERGY;
	}
	// display both energy produced so far today and current power
	if (strcmp(argv[i],"-b")==0){
	    display_flag=DISPLAY_BOTH;
	}
	// help/usage
	if(strcmp(argv[i],"-h")==0){
	    i++;
	    if(i<argc){
		usage(argv[0]);
		return(-1);
	    }
	}
    }

    // check that required parameters were present, or show usage and exit
    if(sbAddrStr[0]=='\x0' || sbSerialStr[0]=='\x0'){
	usage(argv[0]);
	return(-1);
    }

    LOGGER_FMT_INFO("Inverter address:       %s",sbAddrStr);
    LOGGER_FMT_INFO("Inverter serial number: %s",sbSerialStr);

    // open the config file
    LOGGER_FMT_INFO("Reading script from:     %s", scriptFName);
    if( (fp=fopen(scriptFName,"r"))==NULL){
	LOGGER_FMT_ERROR("Could not open script file: %s: %s\n",scriptFName,strerror(errno));
	return -1;
    }
   
    // -----------------------------------------------------------------
    // setup socket
    struct sockaddr_rc addr = { 0 };
    int sb_sock;
    
    sb_sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( sbAddrStr, &addr.rc_bdaddr );
    // connect to server
    if ( connect(sb_sock, (struct sockaddr *)&addr, sizeof(addr)) <0){
	printf("Error connecting to %s\n",sbAddrStr);
	return -1;
    }
    
    // set up socket for select with timeout
    struct timeval tv;
    fd_set readfds;
    
    tv.tv_sec = sb_sock_read_timeout_sec; // set timeout of reading to 5 seconds
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(sb_sock, &readfds);
    // -----------------------------------------------------------------
    
    // convert address - note that inverter protocol uses LSB first
    sb_bt_addr[5] = conv(strtok(sbAddrStr,":"));
    sb_bt_addr[4] = conv(strtok(NULL,":"));
    sb_bt_addr[3] = conv(strtok(NULL,":"));
    sb_bt_addr[2] = conv(strtok(NULL,":"));
    sb_bt_addr[1] = conv(strtok(NULL,":"));
    sb_bt_addr[0] = conv(strtok(NULL,":"));
    
    // convert serial
    serial[3] = conv(strtok(sbSerialStr,":"));
    serial[2] = conv(strtok(NULL,":"));
    serial[1] = conv(strtok(NULL,":"));
    serial[0] = conv(strtok(NULL,":"));

    // Count of the script file line number being processed
    unsigned script_line_num=0;
    // flag used to indicate to script loop that further processiing is not required
    char done_flag=0;

    // loop thru script file
    while (!done_flag && !feof(fp)){
	script_line_num++;
	//read line from script file
	if (fgets(line,400,fp) != NULL){
	   LOGGER_FMT_DEBUG("script[%u] '%s'", script_line_num, line);

	   lineread = strtok(line," ;");
	   if(!strcmp(lineread,"R")){		// The script file R indicates that we are to wait to receive data from sb
	       
	       cc = 0; // char count
	       do{
		   // read line data from script file, and make up data in fl, being the data that
		   // we are expecting to receive from sb
		   lineread = strtok(NULL," ;");
		   switch(select_str(lineread)) {
		       case 0: // $END
			   //do nothing
			   break;			
		       case 1: // $ADDR
			   for (i=0;i<6;i++){
			       fl[cc] = sb_bt_addr[i];
			       cc++;
			   }
			   break;	
		       case 7: // $ADD2
			   for (i=0;i<6;i++){
			       fl[cc] = our_bt_addr[i];
			       cc++;
			   }
			   break;	
		       case 3: // $SER
			   for (i=0;i<4;i++){
			       fl[cc] = serial[i];
			       cc++;
			   }
			   break;	
		       case 8: // $CHAN
			   fl[cc] = chan[0];
			   cc++;
			   break;
		       default :
			   fl[cc] = conv(lineread);
			   cc++;
		   }
	       } while (strcmp(lineread,"$END"));

	       // fl now contains the data that we are expecting to receive from sb, and cc is the number of 
	       // characters inf fl
	       log_data_debug("waiting for: ",fl,cc);
	       LOGGER_FMT_DEBUG("matching on %i chars",cc);
	       found = 0;

	       do {
		   // read from socket or timeout
		   int bytes_read=0;
		   memset(received,0,sizeof(received) );

		   select(sb_sock+1, &readfds, NULL, NULL, &tv);
		   if (FD_ISSET(sb_sock, &readfds)){   
		       // data is available to be read from socket
		       if( (bytes_read = recv(sb_sock, buf, sizeof(buf), 0))==-1){
			   // error reading
			   LOGGER_FMT_ERROR("Could not read from socket: %s\n",strerror(errno));
			   return -1;
		       }
		       LOGGER_FMT_DEBUG("received %i bytes", bytes_read);
		       log_data_debug("received:    ", buf, bytes_read);
		   } else {
		       LOGGER_ERROR("Timeout reading bluetooth socket");
		       return -1;
		   }
		   if ( bytes_read > 0){
		       // copy and unescape the data read from socket in buf[] into received[]
		       rr = 0; // count of number of bytes copied into received[]
		       for (i=0;i<bytes_read;i++){ //start copy the rec buffer in to received
			   if (buf[i] == 0x7d){ //did we receive the escape char
			       switch (buf[i+1]){   // act depending on the char after the escape char
				   case 0x5e :
				       received[rr] = 0x7e;
				       break;
				   case 0x5d :
				       received[rr] = 0x7d;
				       break;
				   default :
				       received[rr] = buf[i+1] ^ 0x20;
				       break;
			       }
			       i++;
			   } else  {
			       received[rr] = buf[i];
			   }
			   rr++;
		       }
		   }	
		   // check to see if received data matches the data that we are waiting form
		   if (memcmp(fl,received,cc) == 0){
		       found = 1;
		       LOGGER_DEBUG("found");
		   }
	       }while (found == 0);
	   }
	   // send the data read from the script file to sb
	   if(!strcmp(lineread,"S")){		//See if line is something we need to send
	       cc = 0;
	       do{
		   time_t curtime;
		   lineread = strtok(NULL," ;");
		   switch(select_str(lineread)) {
		       case 0: // $END
			   //do nothing
			   break;			
		       case 1: // $ADDR
			   for (i=0;i<6;i++){
			       fl[cc] = sb_bt_addr[i];
			       cc++;
			   }
			   break;
		       case 7: // $ADD2
			   for (i=0;i<6;i++){
			       fl[cc] = our_bt_addr[i];
			       cc++;
			   }
			   break;
		       case 2: // $TIME	
			   // get unix time and convert
			   curtime= time(NULL);  //get time in seconds since epoch (1/1/1970)	
			   sprintf(tt,"%X",(int)curtime); //convert to a hex in a string
			   for (i=9;i>0;i=i-2){ //change order and convert to integer
			       ti[1] = tt[i];
			       ti[0] = tt[i-1];	
			       ti[2] = 0;
			       fl[cc] = conv(ti);
			       cc++;		
			   }
			   break;
		       case 4: //$crc
			   tryfcs16(fl+19, cc -19);
			   break;
		       case 8: // $CHAN
			   fl[cc] = chan[0];
			   cc++;
			   break;
		       default :
			   fl[cc] = conv(lineread);
			   cc++;
		   }
	       } while (strcmp(lineread,"$END"));
	       log_data_debug("send ", fl,cc);
	       // write data to socket
	       if(write(sb_sock,fl,cc)==-1){
		   LOGGER_FMT_ERROR("Could not write to socket: %s\n",strerror(errno));
		   return -1;
	       }
	   }
	   
	   if(!strcmp(lineread,"E")){		//See if line is something we need to extract
	       LOGGER_DEBUG("Extracting");
	       cc = 0;
	       do{
		   lineread = strtok(NULL," ;");
		   switch(select_str(lineread)) {
		       case 5: // extract current power
			   memcpy(pac,received+67,2);
			   currentpower = (pac[1] * 256) + pac[0];
			   LOGGER_FMT_INFO("power (W): %i",currentpower);
			   // -b or -d flag was not specified (ie only power is required), then our work is done
			   if(display_flag==DISPLAY_POWER){
			       done_flag=1;
			   }
			   break;
		       case 6: // extract total energy collected today
			   memcpy(tot,received+83,2);
			   dtotal = (tot[1] * 256) + tot[0];
			   dtotal = dtotal / 1000;
			   LOGGER_FMT_INFO("energy_today (kWh): %.2f",dtotal);
			   break;		
		       case 7: // extract 2nd address, ie our address
			   memcpy(our_bt_addr,received+26,6);
			   LOGGER_INFO("got our bt address: ");
			   break;
		       case 8: // extract bluetooth channel
			   memcpy(chan,received+22,1);
			   LOGGER_FMT_INFO("bluetooth channel: %i",chan[0]);
			   break;
		   }				
	       } while (strcmp(lineread,"$END"));
	   } 

	} else 	{
	    // there was an error reading line from script file
	    LOGGER_FMT_ERROR("Failed to read line from script file at line %u", script_line_num);
	}
    }
   
    // display results
    if(display_flag== DISPLAY_BOTH)
	printf("%i,%.2f\n",currentpower,dtotal);
    else if(display_flag== DISPLAY_POWER)
	printf("%i\n",currentpower);
    else if(display_flag== DISPLAY_ENERGY)
	printf("%.2f\n",dtotal);

   // close script file
   fclose(fp);

   // close socket
   close(sb_sock);
   return 0;
}

