#include <stdio.h>
#include <pcap.h>		// IMPORTANT
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>		// Use for atoi

typedef struct eth_hdr	// Ethernet Header
{
    u_char dst_mac[6];
    u_char src_mac[6];
    u_short eth_type;
}eth_hdr;
eth_hdr *ethernet;

typedef struct ip_hdr	// IPv4 Header
{
    int version:4;
    int header_len:4;
    u_char tos:8;
    int total_len:16;
    int ident:16;
    int flags:16;
    u_char ttl:8;
    u_char protocol:8;
    int checksum:16;
    u_char sourceIP[4];
    u_char destIP[4];
}ip_hdr;
ip_hdr *ip;

typedef struct tcp_hdr	// TCP Header
{
    u_short sport;
    u_short dport;
    u_int seq;
    u_int ack;
    u_char head_len;
    u_char flags;
    u_short wind_size;
    u_short check_sum;
    u_short urg_ptr;
}tcp_hdr;
tcp_hdr *tcp;

typedef struct udp_hdr	// UDP Header
{
    u_short sport;
    u_short dport;
    u_short tot_len;
    u_short check_sum;
}udp_hdr;
udp_hdr *udp;


// Callback Function
void pcap_callback(unsigned char * arg, const struct pcap_pkthdr *packet_header, const unsigned char *packet_content){
    static int id = 1;
    printf("Packet ID: %d\n", id++);
    pcap_dump(arg, packet_header, packet_content);
    printf("Packet Length: %d\n", packet_header->len);
    printf("Number of Bytes: %d\n", packet_header->caplen);
    printf("Received Time: %s\n", ctime((const time_t*)&packet_header->ts.tv_sec));
    int i;
    for (i = 0; i < packet_header->caplen; i++){
        printf(" %02x", packet_content[i]);
        if ((i + 1) % 32 == 0){
            printf("\n");
        }
    }
    printf("\n\n");

    u_int eth_len = sizeof(struct eth_hdr);
    u_int ip_len = sizeof(struct ip_hdr);
    u_int tcp_len = sizeof(struct tcp_hdr);
    u_int udp_len = sizeof(struct udp_hdr);

    printf("Analyse Result:\n\n");

    printf("Ethernet Header Information:\n");
    ethernet = (eth_hdr *)packet_content;
    printf("Source: %02x:%02x:%02x:%02x:%02x:%02x\n",ethernet->src_mac[0],ethernet->src_mac[1],ethernet->src_mac[2],ethernet->src_mac[3],ethernet->src_mac[4],ethernet->src_mac[5]);
    printf("Destination: %02x:%02x:%02x:%02x:%02x:%02x\n",ethernet->dst_mac[0],ethernet->dst_mac[1],ethernet->dst_mac[2],ethernet->dst_mac[3],ethernet->dst_mac[4],ethernet->dst_mac[5]);
    // printf("Type: %u\n",ethernet->eth_type);

    if(ntohs(ethernet->eth_type)==0x0800){
    	printf("Type: IPv4\n");
        // printf("IPV4 is used.\n");
        printf("IPV4 Header Information:\n");
        ip = (ip_hdr*)(packet_content+eth_len);
        printf("Source: %d.%d.%d.%d\n",ip->sourceIP[0],ip->sourceIP[1],ip->sourceIP[2],ip->sourceIP[3]);
        printf("Destination: %d.%d.%d.%d\n",ip->destIP[0],ip->destIP[1],ip->destIP[2],ip->destIP[3]);
        // if(ip->protocol==6){
        //     printf("tcp is used:\n");
        //     tcp=(tcp_hdr*)(packet_content+eth_len+ip_len);
        //     printf("tcp source port : %u\n",tcp->sport);
        //     printf("tcp dest port : %u\n",tcp->dport);
        // }
        // else if(ip->protocol==17){
        //     printf("udp is used:\n");
        //     udp=(udp_hdr*)(packet_content+eth_len+ip_len);
        //     printf("udp source port : %u\n",udp->sport);
        //     printf("udp dest port : %u\n",udp->dport);
        // }
        // else {
        //     printf("other transport protocol is used\n");
        // }
    }
    else {
    	printf("Type: IPv6\n");
        // printf("IPV6 is used.\n");
    }

    printf("\n%s\n\n", "--------------- NEXT ---------------");
}

int main(int argc, char *argv[])
{
    char *dev, errbuf[1024];

    // Choose network device
    dev = argv[1];
    if (dev == NULL) {
        printf("You didn't provide any NETWORK DEVICE.\n");
        return 0;
    }
    printf("Device: %s\n", dev);

    // Open network device
    pcap_t *pcap_handle = pcap_open_live(dev, 65535, 1, 0, errbuf);

    if (pcap_handle == NULL) {
        printf("%s\n", errbuf);
        return 0;
    }

    // Get IP Address and Netmask
    struct in_addr addr;
    bpf_u_int32 ipaddr, ipmask;
    char *dev_ip, *dev_mask;
    if (pcap_lookupnet(dev, &ipaddr, &ipmask,errbuf) == -1) {
        printf("%s\n", errbuf);
        return 0;
    }

    // Print IP Address
    addr.s_addr = ipaddr;
    dev_ip = inet_ntoa(addr);
    printf("IP Address : %s\n", dev_ip);

    // Print Netmask
    addr.s_addr = ipmask;
    dev_mask = inet_ntoa(addr);
    printf("Netmask : %s\n", dev_mask);

    // BPF Filter
    struct bpf_program filter;
    pcap_compile(pcap_handle, &filter, "icmp[icmptype] == icmp-echoreply or icmp[icmptype] == icmp-echo", 1, 0);
    pcap_setfilter(pcap_handle, &filter);

    // Enter Loop and Open Dump
    printf("\n---------------The PACKET U Get--------------\n\n");
    int id = 0; // ID
    char *num = argv[2];
    // char *rule = argv[3];
    if (num == NULL) {
        printf("You didn't provide Packet number.\n");
        return 0;
    }
    int pnum = atoi(num);
    pcap_dumper_t* dumpfp = pcap_dump_open(pcap_handle, "./log.pcap");
    // Receive [pnum] packets
    if (pcap_loop(pcap_handle, pnum, pcap_callback, (unsigned char *)dumpfp) < 0){
        printf("error\n");
        return 0;
    }

    pcap_dump_close(dumpfp);

    pcap_close(pcap_handle);

    printf("None Packet Left.\n\n---------------All Printed--------------\n\n");

    return 0;
}
// COMPILE: gcc -o pcap pcap.c -lpcap
// USAGE: ./pcap eth0 10