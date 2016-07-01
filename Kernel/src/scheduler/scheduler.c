#include <scheduler.h>
#include <output.h>

static Scheduler scheduler;
static int schedulerInitiated = 0;

extern void _interrupt_20();
extern void interrupt_set(void);
extern void interrupt_clear(void);

Process getProcessWithSpecifiedQueue(int pid, SchedulerLL q);
void printProcessesWithSpecifiedQueue(SchedulerLL q);
int isDummy(LLnode node);
int addDummyProcess();

Process lastProcess = NULL;

void schedulerInit(){
	scheduler = kmalloc(sizeof(struct scheduler));
	scheduler -> waitingpq = queueInit();
	scheduler -> blockedpq = queueInit();
	addDummyProcess();
	schedulerInitiated = 1;
}

Process schedule(){
	Process p;
	if(scheduler -> waitingpq -> current != NULL){
		if(isDummy(scheduler -> waitingpq -> current -> next) || scheduler -> waitingpq -> current -> next -> process -> state != WAITING)
			scheduler -> waitingpq -> current = scheduler -> waitingpq -> current -> next;
		p = scheduler -> waitingpq -> current -> next -> process;
		scheduler -> waitingpq -> current = scheduler -> waitingpq-> current -> next;
		lastProcess = p;
		return p;	
	}
	return NULL;
}

SchedulerLL queueInit(){
	SchedulerLL q = kmalloc(sizeof(struct schedulerLL));
	q -> size = 0;
	q -> current = NULL;
	return q;
}


int addProcess(Process p, SchedulerLL q){
	if(p == NULL)
		return ERROR;
	if(q == NULL)
		return ERROR;
	LLnode node = kmalloc(sizeof(struct llnode));
	if(node == NULL)
		return ERROR; //error

	if(q->size == 0){
		q -> current = node;
		node -> process = p;
		node -> next = node;
		node -> prev = node;
		q -> size++;
		return 1;
	}
	node->process = p;
	q->current->prev ->next = node;
	node -> prev = q -> current -> prev;
	node -> next = q -> current;
	q -> current -> prev = node;
	q->size++;
	return 1; 
}

int addProcessWaiting(Process p){
	return addProcess(p, scheduler -> waitingpq);
}

int addProcessBlocked(Process p){
	return addProcess(p, scheduler -> blockedpq);
}

Process removeProcess(uint64_t pid, SchedulerLL q){
	int found = 0;
	LLnode node = q -> current;
	Process p = NULL;
	if(q->size < 1){
		return NULL;
	}
	do{
		if(node->process->pid == pid)
			found = 1;
		if(!found)
			node = node->next;
	}while(node != q ->current && !found);

	if(q->size == 1 && found){
		p = node -> process; 
		q->size --;
		q -> current = NULL;
		// TODO: free node
		return p;
	}
	if(found){
		p = node->process;
		node -> prev -> next = node -> next;
		node -> next -> prev = node -> prev;
		q->size --;
		// TODO: free node
		return p;
	}
	return NULL;
}

Process removeProcessWaiting(uint64_t pid){
	return removeProcess(pid, scheduler -> waitingpq);
}

Process removeProcessBlocked(uint64_t pid){
	return removeProcess(pid, scheduler -> blockedpq);
}

Process getCurrentWaiting(){
	if(schedulerInitiated == 0)
		return NULL;
	return getCurrent(scheduler -> waitingpq);
}

Process getCurrentBlocked(){
	if(schedulerInitiated == 0)
		return NULL;
	return getCurrent(scheduler -> blockedpq);
}

Process getCurrent(SchedulerLL q){
	if(q->size == 0)
		return NULL;
	return q -> current -> process;
}

int getCurrentPid(){
	Process p = getCurrentWaiting();
	if(p == NULL)
		return -1;
	return getCurrentWaiting() -> pid;
}

Process getProcessWithSpecifiedQueue(int pid, SchedulerLL q){
	int found = 0;
	LLnode node = q -> current;
	if(q->size < 1){
		return NULL;
	}
	do{
		if(node->process->pid == (uint64_t)pid)
			found = 1;
		else
			node = node->next;
	}while(node != q ->current && !found);

	if(found)
		return node->process;
	return NULL;
}

Process getProcess(int pid){
	Process ret = getProcessWithSpecifiedQueue(pid, scheduler->waitingpq);
	if(ret != NULL)
		return ret;
	return getProcessWithSpecifiedQueue(pid, scheduler->blockedpq);
}

int isWaiting(int pid){
	return getProcessWithSpecifiedQueue(pid, scheduler->waitingpq) != NULL;
}

int isBlocked(int pid){
	return getProcessWithSpecifiedQueue(pid, scheduler->blockedpq) != NULL;
}

void printProcessesWithSpecifiedQueue(SchedulerLL q){
	
	LLnode node = q -> current;
	char* pState;
	if(q->size < 1){
		return;
	}
	do{
		if(!isDummy(node))
			out_printf("Name: %s\t PID: %d\t State: %s\tFather: %d\tReserved: %h\n", node->process->name, node->process->pid, states[node->process->state], node->process->father, node->process->reserved);
		node = node->next;
	}while(node->process->pid != q ->current->process->pid);
}

void printProcesses(){
	printProcessesWithSpecifiedQueue(scheduler->waitingpq);
	printProcessesWithSpecifiedQueue(scheduler->blockedpq);
}

void endProcess(){
	int pid = getCurrent(scheduler->waitingpq) -> pid;
	killProcess(pid);
}

int killProcess(int pid){
	if(pid < 1)
		return 0;
	Process p = removeProcessWaiting(pid);
	if(p == NULL)
		p = removeProcessBlocked(pid);
	if(p == NULL)
		return 0;
	//TODO: free	
	return 1;
}

int addDummyProcess(){
	return create_process("dummy", NULL, 0, 0);
}

int isDummy(LLnode node){
	return !(node->process->pid);
}

int blockProcess(int pid){
	interrupt_clear();
	Process p = removeProcessWaiting(pid);
	if(p == NULL)
		return -1;
	p->state = BLOCKED;
	int ret = addProcessBlocked(p);
	interrupt_set();
	while(isBlocked(pid));
//	_interrupt_20();
	return ret;
//	return addProcessBlocked(p);
}

int unblockProcess(int pid){
	interrupt_clear();
	Process p = removeProcessBlocked(pid);
	if(p == NULL)
		return -1;
	p->state = WAITING;
	interrupt_set();
	return addProcessWaiting(p);
}

Process getLastProcess(){
	return lastProcess;
}