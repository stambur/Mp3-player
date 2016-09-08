#include "getpath.h"

////////////// return usb flash memory path ///////////////////////////
char* usbPath()
{ 
    char* path = NULL;
    struct mntent *ent;
    FILE *aFile;
    aFile = setmntent("/proc/mounts", "r");
    if (aFile == NULL) {
        perror("setmntent");
        exit(1);
    }
    while (NULL != (ent = getmntent(aFile))) {
        if(strstr(ent->mnt_fsname,"/dev/sda1")!=NULL) {
            path=ent->mnt_dir;
            break;
        }
        //printf("%s\n", path);
    }
    endmntent(aFile);
    return path;
}

