/* send_arp.c

This program sends out one ARP packet with source/target IP and Ethernet
hardware addresses suuplied by the user.  It compiles and works on Linux
and will probably work on any Unix that has SOCK_PACKET.

The idea behind this program is a proof of a concept, nothing more.  It
comes as is, no warranty.  However, you're allowed to use it under one
condition: you must use your brain simultaneously.  If this condition is
not met, you shall forget about this program and go RTFM immediately.

yuri volobuev'97
volobuev@t1.chem.umn.edu

*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>

#define ETH_HW_ADDR_LEN 6
#define IP_ADDR_LEN 4
#define ARP_FRAME_TYPE 0x0806
#define ETHER_HW_TYPE 1
#define IP_PROTO_TYPE 0x0800
#define OP_ARP_REQUEST 2

#define DEFAULT_DEVICE "eth0"

char usage[]={"Usage: send_arp src_ip_addr src_hw_addr targ_ip_addr tar_hw_addr iface\n\
    \t You can use 'iface' as the HW addr of the iface."};

struct arp_packet {
        u_char targ_hw_addr[ETH_HW_ADDR_LEN];
        u_char src_hw_addr[ETH_HW_ADDR_LEN];
        u_short frame_type;
        u_short hw_type;
        u_short prot_type;
        u_char hw_addr_size;
        u_char prot_addr_size;
        u_short op;
        u_char sndr_hw_addr[ETH_HW_ADDR_LEN];
        u_char sndr_ip_addr[IP_ADDR_LEN];
        u_char rcpt_hw_addr[ETH_HW_ADDR_LEN];
        u_char rcpt_ip_addr[IP_ADDR_LEN];
        u_char padding[18];
};

void die(char *);
void get_ip_addr(struct in_addr*,char*);
void get_hw_addr(char*,char*);

int main(int argc,char** argv){

struct in_addr src_in_addr,targ_in_addr;
struct arp_packet pkt;
struct sockaddr sa;
/** MAC cím tárolásához**/
struct ifreq req;
int sock;

if(argc != 6)die(usage);

sock=socket(AF_INET,SOCK_PACKET,htons(ETH_P_RARP));
if(sock<0){
        perror("socket");
exit(1);
        }

/** Retrieve additional information about iface **/
memset(&req, 0, sizeof(req));
strncpy(req.ifr_name, argv[5], IF_NAMESIZE - 1);
    if (ioctl(sock, SIOCGIFHWADDR, &req) < 0) {
            perror("ioctl");
            exit(EXIT_FAILURE);
        }


pkt.frame_type = htons(ARP_FRAME_TYPE);
pkt.hw_type = htons(ETHER_HW_TYPE);
pkt.prot_type = htons(IP_PROTO_TYPE);
pkt.hw_addr_size = ETH_HW_ADDR_LEN;
pkt.prot_addr_size = IP_ADDR_LEN;
pkt.op=htons(OP_ARP_REQUEST);

get_hw_addr(pkt.targ_hw_addr,argv[4]);
get_hw_addr(pkt.rcpt_hw_addr,argv[4]);

if( strcmp(argv[2],"iface") == 0 ) {
    memcpy(pkt.src_hw_addr,req.ifr_hwaddr.sa_data,ETH_HW_ADDR_LEN);
    memcpy(pkt.sndr_hw_addr,req.ifr_hwaddr.sa_data,ETH_HW_ADDR_LEN);
}
else {
    get_hw_addr(pkt.src_hw_addr,argv[2]);
    get_hw_addr(pkt.sndr_hw_addr,argv[2]);
}

get_ip_addr(&src_in_addr,argv[1]);
get_ip_addr(&targ_in_addr,argv[3]);

memcpy(pkt.sndr_ip_addr,&src_in_addr,IP_ADDR_LEN);
memcpy(pkt.rcpt_ip_addr,&targ_in_addr,IP_ADDR_LEN);

bzero(pkt.padding,18);

strcpy(sa.sa_data,argv[5]);
if(sendto(sock,&pkt,sizeof(pkt),0,&sa,sizeof(sa)) < 0){
        perror("sendto");
        exit(1);
        }
exit(0);
}

void die(char* str){
fprintf(stderr,"%s\n",str);
exit(1);
}

void get_ip_addr(struct in_addr* in_addr,char* str){

struct hostent *hostp;

in_addr->s_addr=inet_addr(str);
if(in_addr->s_addr == -1){
        if( (hostp = gethostbyname(str)))
                bcopy(hostp->h_addr,in_addr,hostp->h_length);
        else {
                fprintf(stderr,"send_arp: unknown host %s\n",str);
                exit(1);
                }
        }
}

void get_hw_addr(char* buf,char* str){

int i;
char c,val;

for(i=0;i<ETH_HW_ADDR_LEN;i++){
        if( !(c = tolower(*str++))) die("Invalid hardware address");
        if(isdigit(c)) val = c-'0';
        else if(c >= 'a' && c <= 'f') val = c-'a'+10;
        else die("Invalid hardware address");

        *buf = val << 4;
        if( !(c = tolower(*str++))) die("Invalid hardware address");
        if(isdigit(c)) val = c-'0';
        else if(c >= 'a' && c <= 'f') val = c-'a'+10;
        else die("Invalid hardware address");

        *buf++ |= val;

        if(*str == ':')str++;
        }
}
