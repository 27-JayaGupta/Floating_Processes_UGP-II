#ifndef CHANGE_PARENT_H
#define CHANGE_PARENT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 100
#define DEVNAME "migration"


#define IOCTL_DETACH _IOWR(MAJOR_NUM, 0, char*)
#define IOCTL_ATTACH _IOWR(MAJOR_NUM, 1, char*)

#endif
