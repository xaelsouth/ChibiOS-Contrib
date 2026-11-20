/**********************************************************************
 * RTOSPlugin_ChibiOS-21.11.js
 *
 * SEGGER Ozone RTOS-awareness plugin for ChibiOS/RT.
 *
 * This is an adaptation of the obsolete ChibiOS plugin shipped with old
 * Ozone versions (which only understood ChibiOS/RT kernel 6.x, where the
 * ready list and registry lived directly in "ch_system") to the current
 * kernel layout: since kernel 7.x (ChibiOS 21.11, CH_KERNEL_VERSION 7.0.6)
 * and 8.x (master), ch_system_t only holds os_instance_t pointers and the
 * ready list / registry moved into the OS instance:
 *
 *   ch_system.instances[0] -> os_instance_t
 *     .rlist  (ready_list_t: pqueue + current thread)
 *     .reglist (registry_t: queue of all threads, linked via thread.rqueue)
 *   thread_t (os/rt/include/chobjects.h):
 *     .hdr.pqueue.{next,prev,prio}  ready-list linkage + priority
 *     .ctx.sp                       saved context (port_intctx on stack)
 *     .wabase / .waend              working area (stack) bounds
 *     .state                        CH_STATE_* (os/rt/include/chschd.h)
 *     .name                         thread name (CH_CFG_USE_REGISTRY)
 *     .rqueue                       registry linkage
 *
 * Usage in Ozone: Project -> RTOS Awareness -> select this file, or copy it
 * to <Ozone>/Plugins/OS/. All structure access goes through Debug.evaluate()
 * with the real types, so no hardcoded offsets are needed.
 *
 * ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio - Apache License 2.0.
 **********************************************************************/

var g_States = [
  "Ready",        /* CH_STATE_READY    0  */
  "Running",      /* CH_STATE_CURRENT  1  */
  "Starting",     /* CH_STATE_WTSTART  2  */
  "Suspended",    /* CH_STATE_SUSPENDED 3 */
  "Queued",       /* CH_STATE_QUEUED   4  */
  "WaitSem",      /* CH_STATE_WTSEM    5  */
  "WaitMtx",      /* CH_STATE_WTMTX    6  */
  "WaitCond",     /* CH_STATE_WTCOND   7  */
  "Sleeping",     /* CH_STATE_SLEEPING 8  */
  "WaitExit",     /* CH_STATE_WTEXIT   9  */
  "WaitOrEvent",  /* CH_STATE_WTOREVT  10 */
  "WaitAndEvent", /* CH_STATE_WTANDEVT 11 */
  "SendMsgQ",     /* CH_STATE_SNDMSGQ  12 */
  "SendMsg",      /* CH_STATE_SNDMSG   13 */
  "WaitMsg",      /* CH_STATE_WTMSG    14 */
  "Terminated"    /* CH_STATE_FINAL    15 */
];

/**********************************************************************
 * getOSName()
 *   Returns the name of the RTOS this plugin supplies support for.
 **********************************************************************/
function getOSName() {
  return "ChibiOS/RT";
}

/**********************************************************************
 * getContextSwitchAddrs()
 *   Returns the addresses of the functions that complete a context switch
 *   (used by Ozone for stack unwinding). On ARMv7-M the switch happens in
 *   the PendSV handler.
 **********************************************************************/
function getContextSwitchAddrs() {
  var aAddrs = [];
  var Addr = Debug.evaluate("&PendSV_Handler");
  if (Addr != undefined) {
    aAddrs.push(Addr);
  }
  return aAddrs;
}

/**********************************************************************
 * GetTaskStackGrowDir()
 *   ARM stacks grow towards lower addresses.
 **********************************************************************/
function GetTaskStackGrowDir() {
  return -1;
}

/**********************************************************************
 * getInstance()
 *   Returns the address of OS instance 0. Kernels >= 7 keep the ready list
 *   and registry in ch_system.instances[0]; older kernels (6.x) had them
 *   directly in ch_system - fall back to that for compatibility.
 **********************************************************************/
function getInstance() {
  var inst = Debug.evaluate("ch_system.instances[0]");
  if (inst == undefined || inst == 0) {
    inst = Debug.evaluate("&ch_system");
  }
  return inst;
}

/**********************************************************************
 * helpers
 **********************************************************************/
function eval(expr) {
  var v = Debug.evaluate(expr);
  return (v == undefined) ? 0 : v;
}

function threadField(tp, field) {
  return eval("((thread_t*)" + tp + ")." + field);
}

function threadName(tp) {
  var n = Debug.evaluate("(char*)((thread_t*)" + tp + ").name");
  if (n == undefined || n == 0) {
    return "0x" + tp.toString(16);
  }
  return n;
}

function threadPrio(tp) {
  /* thread's own priority - valid in any state (kernel 7.x / 8.x).
     NOTE: hdr.pqueue.prio is only meaningful while the thread is linked in
     the ready list (the hdr union holds list/queue linkage otherwise), so
     it must NOT be used as the general priority source. */
  var p = Debug.evaluate("((thread_t*)" + tp + ").realprio");
  if (p == undefined) {
    /* effective (possibly inherited) priority, ready-list entry */
    p = Debug.evaluate("((thread_t*)" + tp + ").hdr.pqueue.prio");
  }
  if (p == undefined) {
    p = Debug.evaluate("((thread_t*)" + tp + ").prio");   /* kernel 6.x */
  }
  return (p == undefined) ? 0 : p;
}

function threadState(tp) {
  var s = threadField(tp, "state");
  if (s < 0 || s >= g_States.length) {
    return "?(" + s + ")";
  }
  return g_States[s];
}

/**********************************************************************
 * AddThread()
 *   Adds one thread to the Ozone Threads window.
 *   CurrAddr: address of the currently running thread (undefined if none).
 **********************************************************************/
function AddThread(tp, CurrAddr) {
  var sName   = threadName(tp);
  var sState  = threadState(tp);
  var prio    = threadPrio(tp);
  var waBase  = threadField(tp, "wabase");
  var waEnd   = threadField(tp, "waend");
  var sStack  = "0x" + waBase.toString(16) + "-0x" + waEnd.toString(16);
  var sID     = "0x" + tp.toString(16);

  if (tp == CurrAddr) {
    sState = "Running";
  }

  /* last argument: thread address, passed to getregs(); undefined for the
     currently executing thread (Ozone uses the live registers then) */
  Threads.add(sName, 0, prio, sState, "", sStack, sID, "", "", "",
              (tp == CurrAddr) ? undefined : tp);
}

/**********************************************************************
 * update()
 *   Called by Ozone to refresh the Threads window. Walks the registry of
 *   OS instance 0 (all threads, any state).
 **********************************************************************/
function update() {
  var inst = getInstance();

  Threads.clear();

  if (inst == undefined || inst == 0) {
    return;
  }

  var CurrAddr = eval("((os_instance_t*)" + inst + ").rlist.current");

  /* registry head and link offset of thread.rqueue */
  var head  = eval("(int)&((os_instance_t*)" + inst + ").reglist");
  var qoff  = eval("(int)&((thread_t*)0)->rqueue");
  var q     = eval("((os_instance_t*)" + inst + ").reglist.queue.next");

  var guard = 0;
  while (q != 0 && q != head && guard < 1000) {
    var tp = q - qoff;
    AddThread(tp, CurrAddr);
    q = eval("((ch_queue_t*)" + q + ")->next");
    guard++;
  }
}

/**********************************************************************
 * getregs()
 *   Returns the register set (R0..R15, PSR) of a non-current thread.
 *   On ARMv7-M the context is stacked: thread.ctx.sp points to the
 *   software-saved part (port_intctx: r4-r11, plus FPU s16-s31 if the FPU
 *   context is saved), immediately followed by the hardware exception
 *   frame (r0,r1,r2,r3,r12,lr,pc,xpsr).
 **********************************************************************/
function getregs(hTask) {
  var aRegs = new Array(17);
  var i;
  for (i = 0; i < 17; i++) {
    aRegs[i] = 0;
  }

  var sp = eval("((thread_t*)" + hTask + ").ctx.sp");
  if (sp == 0) {
    return aRegs;
  }

  /* software-saved registers r4..r11 */
  var names = ["r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11"];
  for (i = 0; i < 8; i++) {
    aRegs[4 + i] = eval("((thread_t*)" + hTask + ").ctx.sp->" + names[i]);
  }

  /* hardware exception frame follows port_intctx in memory */
  var frame = sp + eval("sizeof(struct port_intctx)");
  aRegs[0]  = eval("*(unsigned int*)" + frame);          /* r0  */
  aRegs[1]  = eval("*(unsigned int*)" + (frame + 4));    /* r1  */
  aRegs[2]  = eval("*(unsigned int*)" + (frame + 8));    /* r2  */
  aRegs[3]  = eval("*(unsigned int*)" + (frame + 12));   /* r3  */
  aRegs[12] = eval("*(unsigned int*)" + (frame + 16));   /* r12 */
  aRegs[14] = eval("*(unsigned int*)" + (frame + 20));   /* lr  */
  aRegs[15] = eval("*(unsigned int*)" + (frame + 24));   /* pc  */
  aRegs[16] = eval("*(unsigned int*)" + (frame + 28));   /* xpsr */
  aRegs[13] = frame;                                     /* sp  */

  return aRegs;
}
