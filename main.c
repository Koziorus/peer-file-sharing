#include <time.h>
#include "helper.h"
#include "tracker_communication.h"

int main(int argc, unsigned char *argv[])
{
    srand(time(NULL)); // for random peer_id generation

    FILE* torrent_file;
    torrent_file = fopen("debian-12.6.0-i386-netinst.iso.torrent", "r");
    
    if(torrent_file == NULL)
    {
        failure("open");
    }
    
    unsigned char bencoded_torrent[MAX_BENCODED_TORRENT_LEN];
    fread(bencoded_torrent, sizeof(unsigned char), MAX_BENCODED_TORRENT_LEN, torrent_file);
    fclose(torrent_file);

    printf("%s\n", bencoded_torrent);
    b_print(bencoded_torrent);

    get_tracker_info(bencoded_torrent, NULL);

    return 0;
}
    
/*
TODO:
- add checking if HTTP response is "OK" and only then proceed to extract the bencode body
- create a TODO list with tasks to include error/exceptions handling in various parts of the code 
- ! change bencode.c so that it doesnt use checking of '\0' to end loops (except checking for `path`)
*/