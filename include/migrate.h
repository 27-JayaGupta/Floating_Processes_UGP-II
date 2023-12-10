#define MAJOR_NUM 100
#define IOCTL_DETACH _IOWR(MAJOR_NUM, 0, char*)
#define IOCTL_ATTACH _IOWR(MAJOR_NUM, 1, char*)

#define CLONE_ATTACH 0x400000000ULL

int __migrate(pid_t source_pid, pid_t target_pid);