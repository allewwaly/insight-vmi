# The InSight Shell #



# Introduction #

InSight provides a shell-like interface for [processing debugging symbols](DebugSymbols.md), loading and comparing memory files, interactive analysis, [script execution](ScriptingEngine.md), etc. The shell can be invoked either by connecting with the `insight` program to a running `insightd` daemon, or by running `insightd` interactively (i. e., without the `--daemon` parameter, see RunningInSight).

# Shell Basics #

The shell comes with readline support and further convenience functions you might be familiar with from your favorite shell. The following list summarizes these features:
  * **command history**: Use up and down arrows to browse through previous commands.
  * **history search**: Use CTRL+R to search the command histroy.
  * **command abbreviation**: Only the prefix of a command is required, as long as the prefix is not ambiguous, for example "`memory query`" may be abbreviated to "`m q`".
  * **interrupt support**: Long operations such as script executions may be aborted with CTRL+C, the shell can be closed with CTRL+D.
  * **pipe support** (_experimental_): The output of one command can be piped to other shell utilities such as `grep`, `sort`, `head`, `tail`, etc.

# Built-in Help #

The shell provides a built-in help for the available commands. If at any point you don't remember the syntax of a command, just try `help` to get a list of available commands:

```
>>> help
The following represents a complete list of valid commands:
  diff        Generates a list of vectors corresponding to type IDs
  exit        Exits the program
  help        Displays some help for a command
  list        Lists various types of read symbols
  memory      Load or unload a memory dump
  script      Executes a QtScript script file
  show        Shows information about a symbol given by name or ID
  stats       Shows various statistics.
  symbols     Allows to load, store or parse the kernel symbols
```

To get further help for a particular command, enter `help <command>`, for example:

```
>>> help symbols
Command: symbols
Description: This command allows to load, store or parse the kernel debugging symbols that are to be used.
  symbols parse <kernel_src>     Parse the symbols from a kernel source
                                 tree. Uses "vmlinux" and "System.map"
                                 from that directory.
  symbols parse <objdump> <System.map> <kernel_headers>
                                 Parse the symbols from an objdump output, a
                                 System.map file and a kernel headers dir.
  symbols store <ksym_file>      Saves the parsed symbols to a file
  symbols save <ksym_file>       Alias for "store"
  symbols load <ksym_file>       Loads previously stored symbols for usage
```

# Required Data for Analysis #

## Debugging Symbols ##

Before any memory dump can be loaded, the corresponding debugging symbols have to be loaded first. More information on how InSight can be used to parse the debugging symbols for each supported operating system is described on the DebugSymbols page.

The debugging symbols to be loaded can be specified from [command line](RunningInSight#Command_Line_Parameters_for_insightd.md) or within the shell using the `symbols load` command. The given file can have an absolute path or a path relative to the current working directory. After successfully loading the symbols, some statistics about the types is displayed:

```
>>> symbols load /path/to/insight-2.6-2.6.32.ksym
Reading symbols finished (18621244 bytes read).
Statistics:
  | No. of types:                   41786         
  | No. of types by name:           32298         
  | No. of types by ID:           1370492         
  | No. of types by hash:           40150         
  | No of "struct list_head":         484         
  | No of "struct hlist_node":         46         
  | No. of variables:               24508         
  | No. of variables by ID:         24508         
  | No. of variables by name:       24508         
  | Empty structs remaining:           94         
  `-------------------------------------------
Reading of 18621244 bytes finished in 3 sec (4853073 byte/s).
```

**Note:** Only one set of debugging symbols can be loaded at a time.

## Memory Files ##

InSight reads kernel objects from the physical memory of the machine being analyzed. The physical memory must be provided as a file. This can be a regular file, for example, a memory snapshot taken with KVM or QEMU using the `pmemsave` command, or it can be a special device or memory mapped file provided by the hypervisor to allow direct access to the guest physical memory. One or more memory files to be loaded can be specified at the [command line](RunningInSight#Command_Line_Parameters_for_insightd.md). In addition, they can be loaded and unloaded using the `memory load` and `memory unload` commands of the shell.

```
>>> memory load 20110713-170436.bin
Loaded [0] 20110713-170436.bin
>>> memory load 20110713-170601.bin
Loaded [1] 20110713-170601.bin
>>> memory unload 0
Unloaded [0] 20110713-170436.bin
```

The `memory list` command prints a list of currently loaded memory files:

```
>>> memory list
Loaded memory dumps:
  [1] 20110713-170601.bin
```

# Analyzing the Data #

Once all required data has been loaded, it can be analyzed interactively in various ways.

## Kernel Memory Specifications ##

As described in [How It Works](About#How_it_works.md), InSight requires several constants for virtual-to-physical address translation that it collects when parsing the debugging symbols. The command `memory specs` lists these values:

```
>>> memory specs
ARCHITECTURE          = x86_64
sizeof(unsigned long) = 8
PAGE_OFFSET           = 0xffff880000000000
VMALLOC_START         = 0xffffc90000000000
VMALLOC_END           = 0xffffe8ffffffffff
VMEMMAP_START         = 0xffffea0000000000
VMEMMAP_END           = 0xffffeaffffffffff
MODULES_VADDR         = 0xffffffffa0000000
MODULES_END           = 0xffffffffff000000
START_KERNEL_map      = 0xffffffff80000000
init_level4_pgt       = 0xffffffff81001000
```

This information is mostly interesting to developers.

## Type Information ##

The type database InSight has loaded can be inspected with the `list` command:

```
>>> help list
Command: list
Description: This command lists various types of read symbols.
  list sources              List all source files
  list types [<glob>]       List all types, optionally filtered by a
                            wildcard expression <glob>
  list types-using <id>     List the types using type <id>
  list types-by-id          List the types-by-ID hash
  list types-by-name        List the types-by-name hash
  list variables [<glob>]   List all variables, optionally filtered by
                            a wildcard expression <glob>
```

The commands `list variables` and `list types` are self-explaining and will be the most interesting for users. They both accept an optional wildcard expression as argument to show only those variables or types with matching names. The wildcard expressions follows the usual rules of file wildcards:

  * a `?` matches exactly one arbitrary character
  * a `*` matches zero or more characters
  * character ranges like `[a-f]` match exactly one character within that range
  * all other characters are matched literally

Note that these commands might produce very long lists, it may be advisable [pipe the output through filters](#Shell_Basics.md) such as `grep` in addition to the wildcards.

In addition, the `show` command can be used to display details about a type or variable, either by using their hexadecimal ID or their name. For example, it can be used to output information about the global variable "`modules`":

```
>>> show modules
Found variable with name modules:
  ID:             0x8a6918
  Name:           modules                                                                           
  Address:        0xffffffff8147f7a0                                                                
  Type ID:        0xffffff24                                                                        
  Type:           struct list_head                                                                  
  Source file:    kernel/module.c:81
Corresponding type information:
  ID:             0xffffff24
  Name:           struct list_head                                                                  
  Type:           Struct
  Size:           16
  Source file:    arch/x86/kernel/head64.c:19
  Members:        2
    0x0  next: 0x2b7a struct list_head *
           <1> 0xc30a struct module *: ((struct list_head).next - 8)
    0x8  prev: 0x2b7a struct list_head *
```

Note that the member `next` has an alternative type. While it was originally declared as `struct list_head*`, it is in fact used as `struct module*` with an additional offset of -8. This is reflected in the line starting with `<1>`.

Some types reference further types. The following example shows how to show details of the base type for chained type definitions:

```
>>> show pid_t
Found type with name pid_t:
  ID:             0x231
  Name:           pid_t                                                                             
  Type:           Typedef
  Size:           4
  Source file:    arch/x86/kernel/head64.c:26
  1. Ref.type ID: 0x1ad
  1. Ref.type:    __kernel_pid_t                                                                    
  2. Ref.type ID: 0xc8
  2. Ref.type:    int                                                                               
>>> show 0xc8
Found type with ID 0xc8:
  ID:             0xc8
  Name:           int                                                                               
  Type:           Int32
  Size:           4
```

## Memory Analysis ##

The purpose of InSight is to make kernel objects available for analysis. The InSight shell can be used to inspect the kernel objects interactively. This is very helpful to identify object relations and important information for a particular application. The command to access kernel objects is `memory query`, optionally followed by the index of the memory file to read the object from. If the index is omitted, the first memory file will be used.

### Retrieving Kernel Objects from Global Variables ###

The following example displays the variable `init_tasks` from the first memory file (because no index is given). This variable represents the head to the list of processes on a Linux system. The first first line shows that variable `init_task` is of type **`struct task_struct`** which has the **ID `0x11ba`**. The variable is located at **virtual address `0xffffffff8144b1f0`**. The following lines list all members of `init_task`, their **index**, **offsets**, **names**, **types**, and **values**.

```
>>> memory query init_task
init_task: struct task_struct (ID 0xffffffda) @ 0xffffffff814731f0
  0. 0x000  state                       : volatile long int                = 0
  1. 0x008  stack                       : void *                           = 0xffffffff81418000
  2. 0x010  usage                       : atomic_t                         = ...
  3. 0x014  flags                       : unsigned int                     = 2097408
  4. 0x018  ptrace                      : unsigned int                     = 0
  5. 0x01c  lock_depth                  : int                              = -1
  6. 0x020  prio                        : int                              = 140
  7. 0x024  static_prio                 : int                              = 120
  8. 0x028  normal_prio                 : int                              = 140
  9. 0x02c  rt_priority                 : unsigned int                     = 0
 10. 0x030  sched_class                 : const struct sched_class *       = ... @ 0xffffffff81306dc0                                                                                                   
 11. 0x038  se                          : struct sched_entity              = ...
 12. 0x0e0  rt                          : struct sched_rt_entity           = ...
 13. 0x108  preempt_notifiers           : struct hlist_head                = ...
 14. 0x110  fpu_counter                 : unsigned char                    = 0
 15. 0x114  btrace_seq                  : unsigned int                     = 0
 16. 0x118  policy                      : unsigned int                     = 0
 17. 0x120  cpus_allowed                : cpumask_t                        = ...
 18. 0x160  sched_info                  : struct sched_info                = ...
 19. 0x180  tasks                       : struct list_head                 = ...
 20. 0x190  pushable_tasks              : struct plist_node                = ...
 21. 0x1b8  mm                          : struct mm_struct *               = NULL
 22. 0x1c0  active_mm                   : struct mm_struct *               = ... @ 0xffff88001e099880                                                                                                   
 23. 0x1c8  exit_state                  : int                              = 0
 24. 0x1cc  exit_code                   : int                              = 0
 25. 0x1d0  exit_signal                 : int                              = 0
 26. 0x1d4  pdeath_signal               : int                              = 0
 27. 0x1d8  personality                 : unsigned int                     = 0
 28. 0x1dc  did_exec                    : unsigned int                     = 0
 29. 0x1dc  in_execve                   : unsigned int                     = 0
 30. 0x1dc  in_iowait                   : unsigned int                     = 0
 31. 0x1dc  sched_reset_on_fork         : unsigned int                     = 0
 32. 0x1e0  pid                         : pid_t                            = 0
 33. 0x1e4  tgid                        : pid_t                            = 0
 34. 0x1e8  stack_canary                : long unsigned int                = 10215041519739610821
 35. 0x1f0  real_parent                 : struct task_struct *             = ... @ 0xffffffff814731f0 (self)                                                                                            
 36. 0x1f8  parent                      : struct task_struct *             = ... @ 0xffffffff814731f0 (self)                                                                                            
 37. 0x200  children                    : struct list_head                 = ...
 38. 0x210  sibling                     : struct list_head                 = ...
 39. 0x220  group_leader                : struct task_struct *             = ... @ 0xffffffff814731f0 (self)                                                                                            
 40. 0x228  ptraced                     : struct list_head                 = ...
 41. 0x238  ptrace_entry                : struct list_head                 = ...
 42. 0x248  bts                         : struct bts_context *             = NULL
 43. 0x250  pids                        : struct pid_link[3]               = ...
 44. 0x298  thread_group                : struct list_head                 = ...
 45. 0x2a8  vfork_done                  : struct completion *              = NULL
 46. 0x2b0  set_child_tid               : int *                            = NULL
 47. 0x2b8  clear_child_tid             : int *                            = NULL
 48. 0x2c0  utime                       : cputime_t                        = 0
 49. 0x2c8  stime                       : cputime_t                        = 23
 50. 0x2d0  utimescaled                 : cputime_t                        = 0
 51. 0x2d8  stimescaled                 : cputime_t                        = 23
 52. 0x2e0  gtime                       : cputime_t                        = 0
 53. 0x2e8  prev_utime                  : cputime_t                        = 0
 54. 0x2f0  prev_stime                  : cputime_t                        = 0
 55. 0x2f8  nvcsw                       : long unsigned int                = 0
 56. 0x300  nivcsw                      : long unsigned int                = 5684
 57. 0x308  start_time                  : struct timespec                  = ...
 58. 0x318  real_start_time             : struct timespec                  = ...
 59. 0x328  min_flt                     : long unsigned int                = 0
 60. 0x330  maj_flt                     : long unsigned int                = 0
 61. 0x338  cputime_expires             : struct task_cputime              = ...
 62. 0x350  cpu_timers                  : struct list_head[3]              = ...
 63. 0x380  real_cred                   : const struct cred *              = ... @ 0xffffffff8147ee90                                                                                                   
 64. 0x388  cred                        : const struct cred *              = ... @ 0xffffffff8147ee90                                                                                                   
 65. 0x390  cred_guard_mutex            : struct mutex                     = ...
 66. 0x3b0  replacement_session_keyring : struct cred *                    = NULL
 67. 0x3b8  comm                        : char[16]                         = "swapper"
 68. 0x3c8  link_count                  : int                              = 0
 69. 0x3cc  total_link_count            : int                              = 0
 70. 0x3d0  sysvsem                     : struct sysv_sem                  = ...
 71. 0x3d8  last_switch_count           : long unsigned int                = 0
 72. 0x3e0  thread                      : struct thread_struct             = ...
 73. 0x4a0  fs                          : struct fs_struct *               = ... @ 0xffffffff81487640                                                                                                   
 74. 0x4a8  files                       : struct files_struct *            = ... @ 0xffffffff81487340                                                                                                   
 75. 0x4b0  nsproxy                     : struct nsproxy *                 = ... @ 0xffffffff8147ea70                                                                                                   
 76. 0x4b8  signal                      : struct signal_struct *           = ... @ 0xffffffff81473900                                                                                                   
 77. 0x4c0  sighand                     : struct sighand_struct *          = ... @ 0xffffffff81473cc0                                                                                                   
 78. 0x4c8  blocked                     : sigset_t                         = ...
 79. 0x4d0  real_blocked                : sigset_t                         = ...
 80. 0x4d8  saved_sigmask               : sigset_t                         = ...
 81. 0x4e0  pending                     : struct sigpending                = ...
 82. 0x4f8  sas_ss_sp                   : long unsigned int                = 0
 83. 0x500  sas_ss_size                 : size_t                           = 0
 84. 0x508  notifier                    : int (*)(void *) *                = NULL
 85. 0x510  notifier_data               : void *                           = NULL
 86. 0x518  notifier_mask               : sigset_t *                       = NULL
 87. 0x520  audit_context               : struct audit_context *           = NULL
 88. 0x528  loginuid                    : uid_t                            = 4294967295
 89. 0x52c  sessionid                   : unsigned int                     = 4294967295
 90. 0x530  seccomp                     : seccomp_t                        = ...
 91. 0x534  parent_exec_id              : u32                              = 0
 92. 0x538  self_exec_id                : u32                              = 0
 93. 0x53c  alloc_lock                  : spinlock_t                       = ...
 94. 0x540  irqaction                   : struct irqaction *               = NULL
 95. 0x548  pi_lock                     : spinlock_t                       = ...
 96. 0x550  pi_waiters                  : struct plist_head                = ...
 97. 0x570  pi_blocked_on               : struct rt_mutex_waiter *         = NULL
 98. 0x578  journal_info                : void *                           = NULL
 99. 0x580  bio_list                    : struct bio *                     = NULL
100. 0x588  bio_tail                    : struct bio * *                   = NULL
101. 0x590  reclaim_state               : struct reclaim_state *           = NULL
102. 0x598  backing_dev_info            : struct backing_dev_info *        = NULL
103. 0x5a0  io_context                  : struct io_context *              = NULL
104. 0x5a8  ptrace_message              : long unsigned int                = 0
105. 0x5b0  last_siginfo                : siginfo_t *                      = NULL
106. 0x5b8  ioac                        : struct task_io_accounting        = ...
107. 0x5f0  acct_rss_mem1               : u64                              = 0
108. 0x5f8  acct_vm_mem1                : u64                              = 0
109. 0x600  acct_timexpd                : cputime_t                        = 0
110. 0x608  mems_allowed                : nodemask_t                       = ...
111. 0x610  cpuset_mem_spread_rotor     : int                              = 0
112. 0x618  cgroups                     : struct css_set *                 = ... @ 0xffffffff81653340                                                                                                   
113. 0x620  cg_list                     : struct list_head                 = ...
114. 0x630  robust_list                 : struct robust_list_head *        = NULL
115. 0x638  compat_robust_list          : struct compat_robust_list_head * = NULL
116. 0x640  pi_state_list               : struct list_head                 = ...
117. 0x650  pi_state_cache              : struct futex_pi_state *          = NULL
118. 0x658  perf_event_ctxp             : struct perf_event_context *      = NULL
119. 0x660  perf_event_mutex            : struct mutex                     = ...
120. 0x680  perf_event_list             : struct list_head                 = ...
121. 0x690  mempolicy                   : struct mempolicy *               = NULL
122. 0x698  il_next                     : short int                        = 0
123. 0x69c  fs_excl                     : atomic_t                         = ...
124. 0x6a0  rcu                         : struct rcu_head                  = ...
125. 0x6b0  splice_pipe                 : struct pipe_inode_info *         = NULL
126. 0x6b8  delays                      : struct task_delay_info *         = ... @ 0xffff88001f8de000                                                                                                   
127. 0x6c0  dirties                     : struct prop_local_single         = ...
128. 0x6d8  timer_slack_ns              : long unsigned int                = 50000
129. 0x6e0  default_timer_slack_ns      : long unsigned int                = 0
130. 0x6e8  scm_work_list               : struct list_head *               = NULL
131. 0x6f0  trace                       : long unsigned int                = 0
132. 0x6f8  trace_recursion             : long unsigned int                = 0
133. 0x700  stack_start                 : long unsigned int                = 0
```

The values of members are dipslayed as follows:
  * Values of members representing built-in types are displayed inline (e. g., 128. `timer_slack_ns`).
  * Values of nested `struct` or `union` members are hinted with three dots (e. g., 127. `dirties`).
  * Values of `char` pointers or arrays are shown as strings (e. g., 67. `comm`).
  * Values of other pointers or function pointers show the virtual address they point to (e. g., 1. `stack`).
  * Values of pointers to `struct` or `union` types are hinted with three dots followed by the vitual address they point to (e. g., 126. `delays`).

For any kernel object that represents a struct or union, their members can be followed by using the common notation `object.member`. From the `init_task` object, for example, we are able to follow member `cred` (no. 64) which holds the credentials of the owner of the process:

```
>>> memory query init_task.cred
init_task.cred: struct cred (ID 0x628a) @ 0xffffffff81456c90
 0.  0x00  usage            : atomic_t                   = ...
 1.  0x04  uid              : uid_t                      = 0
 2.  0x08  gid              : gid_t                      = 0
 3.  0x0c  suid             : uid_t                      = 0
 4.  0x10  sgid             : gid_t                      = 0
 5.  0x14  euid             : uid_t                      = 0
 6.  0x18  egid             : gid_t                      = 0
 7.  0x1c  fsuid            : uid_t                      = 0
 8.  0x20  fsgid            : gid_t                      = 0
 9.  0x24  securebits       : unsigned int               = 0
10.  0x28  cap_inheritable  : kernel_cap_t               = ...
11.  0x30  cap_permitted    : kernel_cap_t               = ...
12.  0x38  cap_effective    : kernel_cap_t               = ...
13.  0x40  cap_bset         : kernel_cap_t               = ...
14.  0x48  jit_keyring      : unsigned char              = 0
15.  0x50  thread_keyring   : struct key *               = NULL
16.  0x58  request_key_auth : struct key *               = NULL
17.  0x60  tgcred           : struct thread_group_cred * = ... @ 0xffffffff81456d20
18.  0x68  security         : void *                     = NULL
19.  0x70  user             : struct user_struct *       = ... @ 0xffffffff81455ac0
20.  0x78  group_info       : struct group_info *        = ... @ 0xffffffff81456dc0
21.  0x80  rcu              : struct rcu_head            = ...
```

From here we can further follow the member `user` (no. 19):

```
>>> memory query init_task.cred.user
init_task.cred.user: struct user_struct (ID 0x5add) @ 0xffffffff81455ac0
 0.  0x00  __count         : atomic_t                = ...
 1.  0x04  processes       : atomic_t                = ...
 2.  0x08  files           : atomic_t                = ...
 3.  0x0c  sigpending      : atomic_t                = ...
 4.  0x10  inotify_watches : atomic_t                = ...
 5.  0x14  inotify_devs    : atomic_t                = ...
 6.  0x18  epoll_watches   : atomic_t                = ...
 7.  0x20  mq_bytes        : long unsigned int       = 0
 8.  0x28  locked_shm      : long unsigned int       = 0
 9.  0x30  uid_keyring     : struct key *            = NULL
10.  0x38  session_keyring : struct key *            = NULL
11.  0x40  uidhash_node    : struct hlist_node       = ...
12.  0x50  uid             : uid_t                   = 0
13.  0x58  user_ns         : struct user_namespace * = ... @ 0xffffffff81455290
14.  0x60  locked_vm       : atomic_long_t           = ...
```

This chain can be extended to follow arbitrary paths as long as the memory at the referenced addresses is readable (i. e., not swapped out to disk or in user-land address space).

### Choosing from Multiple Candidate Types ###

As you have seen in the `modules` example [above](#Type_Information.md), a structure field may be used by the kernel contradicting to its declared type. This is indicated in the type information with lines starting with `<#>`. If only one such candidate type is available, InSight automatically chooses this one over the originally declared type. The following example shows how `modules.next` is automatically resolved to its alternative type `struct module*` at the correct offset:

```
>>> memory query modules.next
modules.next: struct module (ID 0xbf3c) @ 0xffffffffa0217730
 0. 0x000  state                   : module_state                   = MODULE_STATE_LIVE (0)
 1. 0x008  list                    : struct list_head               = ...
 2. 0x018  name                    : char[56]                       = "loop"
 3. 0x050  mkobj                   : struct module_kobject          = ...
 4. 0x0a8  modinfo_attrs           : struct module_attribute *      = ... @ 0xffff88001da61a00
 5. 0x0b0  version                 : const char *                   = NULL
 6. 0x0b8  srcversion              : const char *                   = NULL
 7. 0x0c0  holders_dir             : struct kobject *               = ... @ 0xffff8800180a4ac0
 8. 0x0c8  syms                    : const struct kernel_symbol *   = ... @ 0xffffffffa0217550
 9. 0x0d0  crcs                    : const long unsigned int *      = 0xffffffffa0217570
10. 0x0d8  num_syms                : unsigned int                   = 2
11. 0x0e0  kp                      : struct kernel_param *          = ... @ 0xffffffffa0217580
12. 0x0e8  num_kp                  : unsigned int                   = 2
13. 0x0ec  num_gpl_syms            : unsigned int                   = 0
14. 0x0f0  gpl_syms                : const struct kernel_symbol *   = NULL
15. 0x0f8  gpl_crcs                : const long unsigned int *      = NULL
16. 0x100  unused_syms             : const struct kernel_symbol *   = NULL
17. 0x108  unused_crcs             : const long unsigned int *      = NULL
18. 0x110  num_unused_syms         : unsigned int                   = 0
19. 0x114  num_unused_gpl_syms     : unsigned int                   = 0
20. 0x118  unused_gpl_syms         : const struct kernel_symbol *   = NULL
21. 0x120  unused_gpl_crcs         : const long unsigned int *      = NULL
22. 0x128  gpl_future_syms         : const struct kernel_symbol *   = NULL
23. 0x130  gpl_future_crcs         : const long unsigned int *      = NULL
24. 0x138  num_gpl_future_syms     : unsigned int                   = 0
25. 0x13c  num_exentries           : unsigned int                   = 0
26. 0x140  extable                 : struct exception_table_entry * = NULL
27. 0x148  init                    : int (*)() *                    = 0xffffffffa021a000
28. 0x150  module_init             : void *                         = NULL
29. 0x158  module_core             : void *                         = 0xffffffffa0215000
30. 0x160  init_size               : unsigned int                   = 0
31. 0x164  core_size               : unsigned int                   = 12139
32. 0x168  init_text_size          : unsigned int                   = 0
33. 0x16c  core_text_size          : unsigned int                   = 8865
34. 0x170  arch                    : struct mod_arch_specific       = ...
35. 0x170  taints                  : unsigned int                   = 0
36. 0x174  num_bugs                : unsigned int                   = 2
37. 0x178  bug_list                : struct list_head               = ...
38. 0x188  bug_table               : struct bug_entry *             = ... @ 0xffffffffa0217434
39. 0x190  symtab                  : Elf64_Sym *                    = ... @ 0xffffffffa0217980
40. 0x198  core_symtab             : Elf64_Sym *                    = ... @ 0xffffffffa0217980
41. 0x1a0  num_symtab              : unsigned int                   = 38
42. 0x1a4  core_num_syms           : unsigned int                   = 38
43. 0x1a8  strtab                  : char *                         = ""
44. 0x1b0  core_strtab             : char *                         = ""
45. 0x1b8  sect_attrs              : struct module_sect_attrs *     = ... @ 0xffff88001e079800
46. 0x1c0  notes_attrs             : struct module_notes_attrs *    = ... @ 0xffff88001faa79c0
47. 0x1c8  percpu                  : void *                         = NULL
48. 0x1d0  args                    : char *                         = ""
49. 0x1d8  tracepoints             : struct tracepoint *            = NULL
50. 0x1e0  num_tracepoints         : unsigned int                   = 0
51. 0x1e8  trace_bprintk_fmt_start : const char * *                 = NULL
52. 0x1f0  num_trace_bprintk_fmt   : unsigned int                   = 0
53. 0x1f8  trace_events            : struct ftrace_event_call *     = NULL
54. 0x200  num_trace_events        : unsigned int                   = 0
55. 0x208  modules_which_use_me    : struct list_head               = ...
56. 0x218  waiter                  : struct task_struct *           = ... @ 0xffff88001e093170
57. 0x220  exit                    : void (*)() *                   = 0xffffffffa0217208
58. 0x228  refptr                  : char *                         = 0x00000000000161f8
59. 0x230  ctors                   : ctor_fn_t *                    = NULL
60. 0x238  num_ctors               : unsigned int                   = 0
```

You can always choose a specific candidate type using an angle bracket suffix with the desired type index for the query expression, for example `memory query modules.next`**`<1>`**:

```
>>> memory query modules.next<1>
modules.next<1>: struct module (ID 0xbf3c) @ 0xffffffffa0217730
[...]
```

In case you want to force the declared type, use angle brackets with an index of zero:

```
>>> memory query modules.next<0>
modules.next<0>: struct list_head (ID 0x2b51) @ 0xffffffffa0217738
0. 0x00  next : struct list_head * = ... @ 0xffffffffa01fe3f8
1. 0x08  prev : struct list_head * = ... @ 0xffffffff8147f7a0
```

### Kernel Objects Type Casts and Custom Offsets ###

InSight also allows to cast the type of kernel objects and subtract a custom offset as follows. We already know that the global variable `modules` of type `list_head` is in fact the head to a list of `struct module` objects. We can still access the modules by casting the type to `struct module` and correcting its address by the offset of member `list` within `struct module`.

The syntax for this output is:

<pre>memory query (<type> [- <member-of-type>|<int>]]) object</pre>

This will cast `object` to type `<type>` (specified by its name or ID), optionally subtracting the specified offset from `object`'s address. If the offset is specified as `<member-of-type>`, InSight will subtract the offset of the given member name within `<type>` from the address. If the offset is specified as `<int>`, that value will be subtracted.

To continue our example, we want to cast `modules.next` to type `struct module` and subtract the offset of `module`'s member `list` from the resulting address. This can be expressed as follows:

```
>>> memory query modules.(module - list)next
modules.(module - list)next: struct module (ID 0xbeea) @ 0xffffffffa02261e0
 0.  0x000  state                   : module_state                   = MODULE_STATE_LIVE (0)
 1.  0x008  list                    : struct list_head               = ...
 2.  0x018  name                    : char[56]                       = "fuse"
 3.  0x050  mkobj                   : struct module_kobject          = ...
 4.  0x0a8  modinfo_attrs           : struct module_attribute *      = ... @ 0xffff880017ca0400
 5.  0x0b0  version                 : const char *                   = NULL
 6.  0x0b8  srcversion              : const char *                   = NULL
 7.  0x0c0  holders_dir             : struct kobject *               = ... @ 0xffff88001f2bbc40
 8.  0x0c8  syms                    : const struct kernel_symbol *   = NULL
 9.  0x0d0  crcs                    : const long unsigned int *      = NULL
10.  0x0d8  num_syms                : unsigned int                   = 0
11.  0x0e0  kp                      : struct kernel_param *          = ... @ 0xffffffffa0226058
12.  0x0e8  num_kp                  : unsigned int                   = 2
13.  0x0ec  num_gpl_syms            : unsigned int                   = 17
14.  0x0f0  gpl_syms                : const struct kernel_symbol *   = ... @ 0xffffffffa0225d40
15.  0x0f8  gpl_crcs                : const long unsigned int *      = 0xffffffffa0225e50
16.  0x100  unused_syms             : const struct kernel_symbol *   = NULL
17.  0x108  unused_crcs             : const long unsigned int *      = NULL
18.  0x110  num_unused_syms         : unsigned int                   = 0
19.  0x114  num_unused_gpl_syms     : unsigned int                   = 0
20.  0x118  unused_gpl_syms         : const struct kernel_symbol *   = NULL
21.  0x120  unused_gpl_crcs         : const long unsigned int *      = NULL
22.  0x128  gpl_future_syms         : const struct kernel_symbol *   = NULL
23.  0x130  gpl_future_crcs         : const long unsigned int *      = NULL
24.  0x138  num_gpl_future_syms     : unsigned int                   = 0
25.  0x13c  num_exentries           : unsigned int                   = 0
26.  0x140  extable                 : struct exception_table_entry * = NULL
27.  0x148  init                    : int (*)() *                    = 0xffffffffa022b059
28.  0x150  module_init             : void *                         = NULL
29.  0x158  module_core             : void *                         = 0xffffffffa021c000
30.  0x160  init_size               : unsigned int                   = 0
31.  0x164  core_size               : unsigned int                   = 49861
32.  0x168  init_text_size          : unsigned int                   = 0
33.  0x16c  core_text_size          : unsigned int                   = 36308
34.  0x170  arch                    : struct mod_arch_specific       = ...
35.  0x170  taints                  : unsigned int                   = 0
36.  0x174  num_bugs                : unsigned int                   = 18
37.  0x178  bug_list                : struct list_head               = ...
38.  0x188  bug_table               : struct bug_entry *             = ... @ 0xffffffffa0225c67
39.  0x190  symtab                  : Elf64_Sym *                    = ... @ 0xffffffffa0226468
40.  0x198  core_symtab             : Elf64_Sym *                    = ... @ 0xffffffffa0226468
41.  0x1a0  num_symtab              : unsigned int                   = 187
42.  0x1a4  core_num_syms           : unsigned int                   = 187
43.  0x1a8  strtab                  : char *                         = ""
44.  0x1b0  core_strtab             : char *                         = ""
45.  0x1b8  sect_attrs              : struct module_sect_attrs *     = ... @ 0xffff88001f296800
46.  0x1c0  notes_attrs             : struct module_notes_attrs *    = ... @ 0xffff88001fbf1a20
47.  0x1c8  percpu                  : void *                         = NULL
48.  0x1d0  args                    : char *                         = ""
49.  0x1d8  tracepoints             : struct tracepoint *            = NULL
50.  0x1e0  num_tracepoints         : unsigned int                   = 0
51.  0x1e8  trace_bprintk_fmt_start : const char * *                 = NULL
52.  0x1f0  num_trace_bprintk_fmt   : unsigned int                   = 0
53.  0x1f8  trace_events            : struct ftrace_event_call *     = NULL
54.  0x200  num_trace_events        : unsigned int                   = 0
55.  0x208  modules_which_use_me    : struct list_head               = ...
56.  0x218  waiter                  : struct task_struct *           = ... @ 0xffff88001e7222b0
57.  0x220  exit                    : void (*)() *                   = 0xffffffffa0224dac
58.  0x228  refptr                  : char *                         = 0x00000000000161c0 (Error reading from virtual address 0x00000000000161c0: address below linear offsets, seems to be user-land memory)
59.  0x230  ctors                   : ctor_fn_t *                    = NULL
60.  0x238  num_ctors               : unsigned int                   = 0
```

First, we inspect `struct module` with the command:

```
>>> show module | grep list_head
    0x0008  list:                 0xfffffdf3 struct list_head
    0x0178  bug_list:             0xfffffdf8 struct list_head
    0x0208  modules_which_use_me: 0xfffffdfd struct list_head
```

This reveals that the `list` member at offset `0x8` most likely is the member that links to the next object. Next, we need to find the address of the first `module` object. We therefore query the global variable `modules`:

```
>>> memory query modules
modules: struct list_head (ID 0x2b5a) @ 0xffffffff814575a0
0.  0x00  next : struct list_head * = ... @ 0xffffffffa02261e8
1.  0x08  prev : struct list_head * = ... @ 0xffffffffa0000918
```


### Retrieving Kernel Objects from Arbitrary Locations ###

For general debugging, the user can request objects of arbitrary types and at any readable location. This is achieved with the command `memory dump` which adheres to the following general syntax:

`memory dump [index] <char|int|long|type-name|type-id>(.<member>)* @ <address>`

This can be used as follows. For example, we know that the global variable `modules` of type `list_head` is in fact the head to a list of `struct module` objects. This is one of the cases InSight cannot automatically handle [yet](RoadMap.md). We can still access the modules by calculating the correct address and requesting an object of type `struct module` at this address

First, we inspect `struct module` with the command:

```
>>> show module | grep list_head
    0x0008  list:                 0xfffffdf3 struct list_head
    0x0178  bug_list:             0xfffffdf8 struct list_head
    0x0208  modules_which_use_me: 0xfffffdfd struct list_head
```

This reveals that the `list` member at offset `0x8` most likely is the member that links to the next object. Next, we need to find the address of the first `module` object. We therefore query the global variable `modules`:

```
>>> memory query modules
modules: struct list_head (ID 0x2b5a) @ 0xffffffff814575a0
0.  0x00  next : struct list_head * = ... @ 0xffffffffa02261e8
1.  0x08  prev : struct list_head * = ... @ 0xffffffffa0000918
```

We see that the `next` pointer points to address `0xffffffffa02261e8`, however we assume that this is the address of member `list` _within_ the `module` object. Consequently, the first object is located at address `0xffffffffa02261e8 - 0x8 = 0xffffffffa02261e0`. So we request an object of type `module` at this address:

```
>>> mem dump module @ 0xffffffffa02261e0
struct module (ID 0xbeea) @ ffffffffa02261e0
 0.  0x000  state                   : module_state                   = MODULE_STATE_LIVE (0)
 1.  0x008  list                    : struct list_head               = ...
 2.  0x018  name                    : char[56]                       = "fuse"
 3.  0x050  mkobj                   : struct module_kobject          = ...
 4.  0x0a8  modinfo_attrs           : struct module_attribute *      = ... @ 0xffff880017ca0400
 5.  0x0b0  version                 : const char *                   = NULL
 6.  0x0b8  srcversion              : const char *                   = NULL
 7.  0x0c0  holders_dir             : struct kobject *               = ... @ 0xffff88001f2bbc40
 8.  0x0c8  syms                    : const struct kernel_symbol *   = NULL
 9.  0x0d0  crcs                    : const long unsigned int *      = NULL
10.  0x0d8  num_syms                : unsigned int                   = 0
11.  0x0e0  kp                      : struct kernel_param *          = ... @ 0xffffffffa0226058
12.  0x0e8  num_kp                  : unsigned int                   = 2
13.  0x0ec  num_gpl_syms            : unsigned int                   = 17
14.  0x0f0  gpl_syms                : const struct kernel_symbol *   = ... @ 0xffffffffa0225d40
15.  0x0f8  gpl_crcs                : const long unsigned int *      = 0xffffffffa0225e50
16.  0x100  unused_syms             : const struct kernel_symbol *   = NULL
17.  0x108  unused_crcs             : const long unsigned int *      = NULL
18.  0x110  num_unused_syms         : unsigned int                   = 0
19.  0x114  num_unused_gpl_syms     : unsigned int                   = 0
20.  0x118  unused_gpl_syms         : const struct kernel_symbol *   = NULL
21.  0x120  unused_gpl_crcs         : const long unsigned int *      = NULL
22.  0x128  gpl_future_syms         : const struct kernel_symbol *   = NULL
23.  0x130  gpl_future_crcs         : const long unsigned int *      = NULL
24.  0x138  num_gpl_future_syms     : unsigned int                   = 0
25.  0x13c  num_exentries           : unsigned int                   = 0
26.  0x140  extable                 : struct exception_table_entry * = NULL
27.  0x148  init                    : int (*)() *                    = 0xffffffffa022b059
28.  0x150  module_init             : void *                         = NULL
29.  0x158  module_core             : void *                         = 0xffffffffa021c000
30.  0x160  init_size               : unsigned int                   = 0
31.  0x164  core_size               : unsigned int                   = 49861
32.  0x168  init_text_size          : unsigned int                   = 0
33.  0x16c  core_text_size          : unsigned int                   = 36308
34.  0x170  arch                    : struct mod_arch_specific       = ...
35.  0x170  taints                  : unsigned int                   = 0
36.  0x174  num_bugs                : unsigned int                   = 18
37.  0x178  bug_list                : struct list_head               = ...
38.  0x188  bug_table               : struct bug_entry *             = ... @ 0xffffffffa0225c67
39.  0x190  symtab                  : Elf64_Sym *                    = ... @ 0xffffffffa0226468
40.  0x198  core_symtab             : Elf64_Sym *                    = ... @ 0xffffffffa0226468
41.  0x1a0  num_symtab              : unsigned int                   = 187
42.  0x1a4  core_num_syms           : unsigned int                   = 187
43.  0x1a8  strtab                  : char *                         = ""
44.  0x1b0  core_strtab             : char *                         = ""
45.  0x1b8  sect_attrs              : struct module_sect_attrs *     = ... @ 0xffff88001f296800
46.  0x1c0  notes_attrs             : struct module_notes_attrs *    = ... @ 0xffff88001fbf1a20
47.  0x1c8  percpu                  : void *                         = NULL
48.  0x1d0  args                    : char *                         = ""
49.  0x1d8  tracepoints             : struct tracepoint *            = NULL
50.  0x1e0  num_tracepoints         : unsigned int                   = 0
51.  0x1e8  trace_bprintk_fmt_start : const char * *                 = NULL
52.  0x1f0  num_trace_bprintk_fmt   : unsigned int                   = 0
53.  0x1f8  trace_events            : struct ftrace_event_call *     = NULL
54.  0x200  num_trace_events        : unsigned int                   = 0
55.  0x208  modules_which_use_me    : struct list_head               = ...
56.  0x218  waiter                  : struct task_struct *           = ... @ 0xffff88001e7222b0
57.  0x220  exit                    : void (*)() *                   = 0xffffffffa0224dac
58.  0x228  refptr                  : char *                         = 0x00000000000161c0 (Error reading from virtual address 0x00000000000161c0: address below linear offsets, seems to be user-land memory)
59.  0x230  ctors                   : ctor_fn_t *                    = NULL
60.  0x238  num_ctors               : unsigned int                   = 0
```

We also could have used the type ID `0xbeea` instead of the name `module`. As the resulting object links to further objects through its `list` member of type `struct list_head`, we can now request further members using the syntax `object.member` as we already did before. No further user intervention is required:

```
>>> mem dump module.name @ 0xffffffffa02261e0
char[56] (ID 0xf7ec) @ ffffffffa02261f8
"fuse"
>>> mem dump module.list.next.name @ 0xffffffffa02261e0
char[56] (ID module) @ ffffffffa0215748
"loop"
>>> mem dump module.list.next.list.next.name @ 0xffffffffa02261e0
char[56] (ID module) @ ffffffffa01fc408
"snd_pcm"
```

# Executing Scripts #

While the analysis within the shell is very helpful for getting an overview of the information that can be retrieved and how it can be accessed, it provides little degree of automation. For a more powerful analysis without user interaction, InSight provides a JavaScript engine to automate the inspection and share repetitive tasks in functions and libraries.

How to write these scripts is covered on the ScriptingEngine page. Use the command `script` to execute a script with a path relative to the current working directory. Any arguments that follow the script name are passed as [arguments to the script](ScriptingEngine#Arguments_to_Scripts.md). The output of the script is written to the shell:

```
>>> script scripts/tasklist.js
===================================================
USER     PID     GID   STATE  COMMAND           ADDRESS           
   0       0       0       0  "swapper"         0xc0340b20        
   0       1       0       1  "init"            0xce182a90        
   0       2       0       1  "migration/0"     0xce182560        
   0       3       0       1  "ksoftirqd/0"     0xce182030        
   0       4       0       1  "watchdog/0"      0xce186a90        
   0       5       0       1  "events/0"        0xce186560        
   0       6       0       1  "khelper"         0xce186030        
   0       7       0       1  "kthread"         0xcffd3a90        
   0       9       0       1  "kblockd/0"       0xcffd3560        
   0      10       0       1  "kacpid"          0xcffd3030        
   0     119       0       1  "pdflush"         0xcfe8aa90        
   0     120       0       1  "pdflush"         0xcfe8a560        
   0     122       0       1  "aio/0"           0xcfed8a90        
   0     121       0       1  "kswapd0"         0xcfe8a030        
   0     708       0       1  "kseriod"         0xcfed8560        
   0    1828       0       1  "kjournald"       0xcfdaa560        
   0    1985       0       1  "udevd"           0xcfdc9030        
   0    3277       0       1  "dhclient3"       0xcc7c0030        
 102    3423     102       2  "syslogd"         0xc9b41030        
   0    3440       0       1  "dd"              0xcfdaa030        
 103    3442     103      65  "klogd"           0xcfdc9560        
   0    3482       0       1  "mysqld_safe"     0xc96b6030        
 100    3546     104       1  "mysqld"          0xc96b6a90        
   0    3547       0      65  "logger"          0xcbe61560        
   0    3639       0       1  "sshd"            0xcc860a90        
   0    3671       0       1  "mdadm"           0xc9b41a90        
   0    3684       0       1  "atd"             0xcc860560        
   0    3694       0       1  "cron"            0xc96b6560        
   0    3721       0       1  "apache2"         0xc1322030        
   0    3739       0       1  "getty"           0xcfdaaa90        
   0    3740       0       1  "getty"           0xce3a1560        
   0    3741       0       1  "getty"           0xc1322a90        
   0    3742       0       1  "getty"           0xcbe61030        
   0    3743       0       1  "getty"           0xc9b41560        
   0    3744       0       1  "getty"           0xcfdc9a90        
  33    3755      33       1  "apache2"         0xcbae4560        
  33    3756      33       1  "apache2"         0xcbae4030        
  33    3757      33       1  "apache2"         0xcc04aa90        
  33    3758      33       1  "apache2"         0xcc04a560        
  33    3759      33       1  "apache2"         0xcc04a030        
```

Remember that you can use the `--eval` [parameter](RunningInSight#Command_Line_Parameters_for_insight.md) to the `insight` front-end to execute scripts from the command line, for example:

```
insight --eval "script /path/to/scripts/tasklist.js"
```

This will print the output of the script to the console where it can be piped to another program or redirected to a file.