#include "libptrace_do.h"

// Initialize the session. Attach to the process and save its
// register state and finds an syscall instruction in the program binary
struct ptrace_do *ptrace_do_init(int pid){
	int retval, status;
	unsigned long peekdata;
	unsigned long i;
	struct ptrace_do *target;
	siginfo_t siginfo;

	struct parse_maps *map_current;


	if((target = (struct ptrace_do *) malloc(sizeof(struct ptrace_do))) == NULL){
		fprintf(stderr, "%s: malloc(%d): %s\n", program_invocation_short_name, \
				(int) sizeof(struct ptrace_do), strerror(errno));
		return(NULL);
	}
	memset(target, 0, sizeof(struct ptrace_do));
	target->pid = pid;


	// Here we test to see if the child is already attached. This may be the case if the child
	// is a willing accomplice, aka PTRACE_TRACEME.
	// We are testing if it is already traced by trying to read data, specifically its last 
	// signal received. If PTRACE_GETSIGINFO is succesfull *and* the last signal recieved was 
	// SIGTRAP, then it's prolly safe to assume this is the PTRACE_TRACEME case.

	memset(&siginfo, 0, sizeof(siginfo));
	if(ptrace(PTRACE_GETSIGINFO, target->pid, NULL, &siginfo)){

		if((retval = ptrace(PTRACE_ATTACH, target->pid, NULL, NULL)) == -1){
			fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
					(int) PTRACE_ATTACH, (int) target->pid, (long unsigned int) NULL, \
					(long unsigned int) NULL, strerror(errno));
			free(target);
			return(NULL);
		}

		if((retval = waitpid(target->pid, &status, 0)) < 1){
			fprintf(stderr, "%s: waitpid(%d, %lx, 0): %s\n", program_invocation_short_name, \
					(int) target->pid, (unsigned long) &status, strerror(errno));
			free(target);
			return(NULL);
		}

		if(!WIFSTOPPED(status)){
			free(target);
			return(NULL);
		}
	}else{
		if(siginfo.si_signo != SIGTRAP){
			fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): Success, but not recently trapped. Program already traced. Aborting!\n", program_invocation_short_name, \
					(int) PTRACE_GETSIGINFO, (int) target->pid, (long unsigned int) NULL, \
					(long unsigned int) &siginfo);
			free(target);
			return(NULL);
		}
	}


    // Save program state
    if((retval = ptrace(PTRACE_GETREGS, target->pid, NULL, &(target->saved_regs))) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_GETREGS, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) &(target->saved_regs), strerror(errno));
		free(target);
		return(NULL);
	}

	// The tactic for performing syscall injection is to fill the registers to the appropriate values for your syscall,
	// then point $rip at a piece of executable memory that contains the SYSCALL instruction.

	// If we came in from a PTRACE_ATTACH call, then it's likely we are on a syscall edge, and can save time by just
	// using the one SIZEOF_SYSCALL addresses behind where we are right now.
	errno = 0;
	peekdata = ptrace(PTRACE_PEEKTEXT, target->pid, (target->saved_regs).rip - SIZEOF_SYSCALL, NULL);

	if(!errno && ((SYSCALL_MASK & peekdata) == SYSCALL)){
		target->syscall_address = (target->saved_regs).rip - SIZEOF_SYSCALL;

	// Otherwise, we will need to start stepping through the various regions of executable memory looking for 
	// a SYSCALL instruction.
	}else{
		if((target->map_head = get_proc_pid_maps(target->pid)) == NULL){
			fprintf(stderr, "%s: get_proc_pid_maps(%d): %s\n", program_invocation_short_name, \
					(int) target->pid, strerror(errno));
			free(target);
			return(NULL);
		}

		map_current = target->map_head;
		while(map_current){

			if(target->syscall_address){
				break;
			}

            // if its a executable region
			if((map_current->perms & MAPS_EXECUTE)){

				for(i = map_current->start_address; i < (map_current->end_address - sizeof(i)); i++){
					errno = 0;
					peekdata = ptrace(PTRACE_PEEKTEXT, target->pid, i, NULL);
					if(errno){
						fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
								(int) PTRACE_PEEKTEXT, (int) target->pid, i, \
								(long unsigned int) NULL, strerror(errno));
						free(target);
						free_parse_maps_list(target->map_head);
						return(NULL);
					}

					if((SYSCALL_MASK & peekdata) == SYSCALL){
						target->syscall_address = i;
						break;
					}
				}
			}

			map_current = map_current->next;
		}
	}
	return(target);
}

// Save tracee's register state
void ptrace_do_get_regs(struct ptrace_do *target) {
    int retval;
    
    if((retval = ptrace(PTRACE_GETREGS, target->pid, NULL, &(target->saved_regs))) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_GETREGS, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) &(target->saved_regs), strerror(errno));
		free(target);
		return;
	}
}

//Restore tracee's saved register state
void ptrace_do_set_regs(struct ptrace_do *target) {
    int retval;
	
	if((retval = ptrace(PTRACE_SETREGS, target->pid, NULL, &(target->saved_regs))) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_SETREGS, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) &(target->saved_regs), strerror(errno));
        
        return;
	}
}

 // Set up and execute a syscall within the remote process.
unsigned long ptrace_do_syscall(struct ptrace_do *target, unsigned long rax, \
		unsigned long rdi, unsigned long rsi, unsigned long rdx, \
		unsigned long r10, unsigned long r8, unsigned long r9){

	int retval, status, sig_remember = 0;
	struct user_regs_struct attack_regs;


	/*
	 * There are two possible failure modes when calling ptrace_do_syscall():
	 *	
	 * 	1) ptrace_do_syscall() fails. In this case we should return -1 
	 *		and leave errno untouched (as it should be properly set when
	 *		the error occurs).
	 *	
	 *	or	
	 *	
	 * 	2) ptrace_do_syscall() is fine, but the remote syscall fails. 
	 *		In this case, we can't analyze the error without being intrusive,
	 *		so we will leave that job to the calling code. We should return the 
	 *		syscall results as it was passed to us in rax, but that may 
	 * 		legitimately be less than 0. As such we should zero out errno to ensure
	 *		the failure mode we are in is clear.
	 */
	errno = 0;

	memcpy(&attack_regs, &(target->saved_regs), sizeof(attack_regs));

	attack_regs.rax = rax;
	attack_regs.rdi = rdi;
	attack_regs.rsi = rsi;
	attack_regs.rdx = rdx;
	attack_regs.r10 = r10;
	attack_regs.r8 = r8;
	attack_regs.r9 = r9;

	attack_regs.rip = target->syscall_address;

	if((retval = ptrace(PTRACE_SETREGS, target->pid, NULL, &attack_regs)) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_SETREGS, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) &attack_regs, strerror(errno));
		return(-1);
	}

RETRY:
	status = 0;
	if((retval = ptrace(PTRACE_SINGLESTEP, target->pid, NULL, NULL)) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_SINGLESTEP, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) NULL, strerror(errno));
		return(-1);
	}

	if((retval = waitpid(target->pid, &status, 0)) < 1){
		fprintf(stderr, "%s: waitpid(%d, %lx, 0): %s\n", program_invocation_short_name, \
				(int) target->pid, (unsigned long) &status, strerror(errno));
		return(-1);
	}

	if(status){
		if(WIFEXITED(status)){
			errno = ECHILD;
			fprintf(stderr, "%s: waitpid(%d, %lx, 0): WIFEXITED(%d)\n", program_invocation_short_name, \
					target->pid, (unsigned long) &status, status);
			return(-1);
		}
		if(WIFSIGNALED(status)){
			errno = ECHILD;
			fprintf(stderr, "%s: waitpid(%d, %lx, 0): WIFSIGNALED(%d): WTERMSIG(%d): %d\n", \
					program_invocation_short_name, target->pid, (unsigned long) &status, \
					status, status, WTERMSIG(status));
			return(-1);
		}
		if(WIFSTOPPED(status)){

			if(target->sig_ignore & 1<<WSTOPSIG(status)){
				goto RETRY;
			}else if(WSTOPSIG(status) != SIGTRAP){
				sig_remember = status;
				goto RETRY;
			}
		}
		if(WIFCONTINUED(status)){
			errno = EINTR;
			fprintf(stderr, "%s: waitpid(%d, %lx, 0): WIFCONTINUED(%d)\n", program_invocation_short_name, \
					target->pid, (unsigned long) &status, status);
			return(-1);
		}
	}

	if((retval = ptrace(PTRACE_GETREGS, target->pid, NULL, &attack_regs)) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_GETREGS, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) &attack_regs, strerror(errno));
		return(-1);
	}

	// Re-deliver any signals we caught and ignored.
	if(sig_remember){
		// Not checking for errors here. This is a best effort to deliver the previous signal state.
		kill(target->pid, sig_remember);
	}

	// Let's reset this to what it was upon entry.
	if((retval = ptrace(PTRACE_SETREGS, target->pid, NULL, &(target->saved_regs))) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_SETREGS, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) &(target->saved_regs), strerror(errno));
		return(-1);
	}

	// Made it this far. Sounds like the ptrace_do_syscall() was fine. :)
	errno = 0;
	return(attack_regs.rax);
}

// Restore the registers of the target process and detach the tracer
void ptrace_do_cleanup(struct ptrace_do *target){

	int retval;
	
	if((retval = ptrace(PTRACE_SETREGS, target->pid, NULL, &(target->saved_regs))) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_SETREGS, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) &(target->saved_regs), strerror(errno));
	}

	if((retval = ptrace(PTRACE_DETACH, target->pid, NULL, NULL)) == -1){
		fprintf(stderr, "%s: ptrace(%d, %d, %lx, %lx): %s\n", program_invocation_short_name, \
				(int) PTRACE_DETACH, (int) target->pid, (long unsigned int) NULL, \
				(long unsigned int) NULL, strerror(errno));
	}

	free(target);
}