#include "process.h"
#include <strings.h>
#include "kalloc.h"
#include <scheduler.h>

void* set_stack_frame(uint64_t *rsp, process_func func, uint64_t argc, void * argv, void* start_func); //TODO: VER SI RETORNA OTRACOSA PARA UQE LO USO

static uint64_t pids = 0;


void* set_stack_frame(uint64_t *rsp, process_func func, uint64_t argc, void * argv, void* start_func){

	stack_frame *r = (stack_frame*) (rsp);
		
	r->gs = 0x001;
	r->fs = 0x002;
	r->r15= 0x003;
	r->r14= 0x004;
	r->r13= 0x005;	
	r->r12= 0x006;
	r->r11= 0x007;
	r->r10= 0x008;
	r->r9= 0x009;
	r->r8= 0x00A;
	r->rsi= argc;
	r->rdi= (uint64_t)func;
	r->rbp= 0x00D;
	r->rdx= (uint64_t)argv;
	r->rcx= 0x00F;
	r->rbx= 0x010;
	r->rax= 0x011;
	
	//iretq hook
	
	r->rip= (uint64_t)start_func;
	r->cs= 0x008;
	r->eflags= 0x202;
	r->rsp= (uint64_t)&(r->base);
	r->ss= 0x000;
	
	r->base= 0x000;
	
	return &(r->gs);
}

uint64_t create_process(const char* name, process_func func, uint64_t argc, void* argv){
	Process p;
	p = kmalloc(sizeof(struct process));
	if(p == NULL){
		return -1;
	}
	strcpy(p->name, name);
	p->pid = ++pids;
	p->state = WAITING;
	uint64_t rsp = (uint64_t) kmalloc(STACK_SIZE);
	p->stackF = (uint64_t) kmalloc(sizeof(struct stack_frame));
	if(rsp == 0 || p->stackF == 0){
		return -1;
	}
	p->stackF = (uint64_t) set_stack_frame(&rsp, func, argc, argv, func);
	addProcessWaiting(p);
	return 0;
}

uint64_t contextSwitch(uint64_t stackFrame){
	Process p = getCurrentWaiting();
	if (p == NULL)
	{
		return 0;
	}
	p->stackF = stackFrame;

	p = schedule();

	if(p == NULL)
		return 0;
	return p -> stackF;
}