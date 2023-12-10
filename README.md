# Floating Processes Across Namespaces

This report contains the report, presentations, library and kernel source code done in our undergraduate project at IITK. There is also a working demo for the implementation in the directory ```ugp-demo```.

The work was a joint work with [Harshit Bansal](https://github.com/harshit-bansal18/).


## Documentation

* Detailed Weekwise documentation can be found [here](https://docs.google.com/document/d/1uRlNbyWEdsfEcaioIjxzC3uScGTCaeSfS1Dxo8-l_B4/edit?usp=sharing).

*  UGP Presentation and Report are present in ```Report_Presentation``` folder.

## Setup Instruction
```diff
- Warning: The setup needs to be done on a Linux VM. Use a KVM etc to setup your virtual machine.
```

### Building Kernel 

Perform these steps inside VM after booting into default kernel.

<b>======================= To build kernel =======================</b>
* Open terminal.

* Clone the repository.
```bash
$ git clone https://git.cse.iitk.ac.in/jayagupta/ugp-ii
```

* Install programs needed for kernel compilation (for ubuntu)
```bash
$ sudo apt-get update
$ sudo apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison
```

* Enter into linux source code directory
```bash
$ cd UGPII/linux-6.1.4/
```

* Generate configuration file
```bash
$ make defconfig
```

* Initiate compilation (Takes around 30 min to 1 hour to finish)
```bash
$ make -j4
```

* Install modules
```bash
$ sudo make modules_install
```

* Install kernel
```bash
$ sudo make install
```

<b>==================== To show boot menu while booting=======================</b>

* If during booting, you want to pick a particular kernel to run, do the following operations. (Needs to be done only once)

* Open grub file. You can use any editor to edit it. Needs sudo permission to edit.
```bash
$ sudo vi /etc/default/grub 
```

* Make the following changes in the file
```c
#GRUB_TIMEOUT_STYLE=hidden //Comment this line by adding ’#’ infront of it.

GRUB_TIMEOUT=30 //Change timeout value from 0 to 30 secs. Let boot menu be shown for 30 secs

```

* Run following command to apply the above changes
```bash
$ sudo update-grub
```

● Reboot

<b>====================== verify the new kernel install====================</b>

Note: You can use the following command to know the kernel version that got booted.
```
$ uname -a
```


## Building Kernel Module

```bash
cd UGP-II/modules
make clean
make
```

## Building library code

```bash
cd UGP-II
make clean
make
```

## Usage

* Include the library ```migrate.h``` in the manager code.

```c
#include "manager.h"
```

* Inorder to build the program, insert the following in ```Makefile``` in ```UGP-II``` folder.

```Makefile
manager: $(SOURCE_DIR_NAME)/manager.c
	$(CC) $(CFLAGS) -I$(INC) -o $(SOURCE_DIR_NAME)/manager $(SOURCE_DIR_NAME)/manager.c $(LIB_OBJ)/migrate.a
```

* Inorder to migrate a process P1 with ```PID p1``` in the global namespace to process P2 with ```PID p2```, use ```__migrate(source_pid, target_pid)```. This function will return the new global pid of the migrated process in the new namespace.

## Instructions to run the demo

* Build the kernel module.

* Build the library code and ugp-demo.

```
cd UGP-II
make
```

* Launch the ```pstree``` to setup the process tree and then launch the ```manager```.

```bash
cd UGP-II/ugp-demo

# Terminal 1
sudo ./pstree

# Terminal 2
sudo ./manager <source_pid_of_migrating_process>
```
