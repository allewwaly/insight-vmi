# Using the Sample Memory Snapshots #



# Introduction #

In order to try out InSight without having to [generate and parse](DebugSymbols.md) the debugging symbols for a virtual machine yourself, we provide some ready-to-use sample data.

# Obtaining the Memory Snapshots #

The sample data can be found in the [downloads](https://code.google.com/p/insight-vmi/downloads/list) section. Just download one or more of the `sample-*` files and extract them to your disk. Each sample archive contains at least two files:

  * a **`linux-x-y-z.sym`** file, containing the symbol information for the guest
  * one or more **`yyyymmdd-hhmmss.bin`** files, the physical memory snapshots of the guest

# Using the Memory Snapshots #

You can start `insightd` interactively and specify to load these files from the command line, for example:

```
insightd -l linux-2.6-2.6.32-amd64.sym -m 20120316-114700.bin
```

InSight will load the symbols file and output some statistics:

```
Reading symbols finished (18544695 bytes read). 
Statistics:
  | No. of types:                   41063         
  | No. of types by name:           32491         
  | No. of types by ID:           1372554         
  | No. of types by hash:           41063         
  | No. of variables:               24514         
  | No. of variables by ID:         24514         
  | No. of variables by name:       24514         
  | Empty structs remaining:           93         
  `-------------------------------------------
Reading of 18544695 bytes finished in 3 sec (4962455 byte/s).
Loaded [0] /tmp/debian-6.0-amd64/20120316-114700.bin
>>> _
```

Now you are ready to use the [shell](InSightShell.md) to query the available types and variables or execute [JavaScript code](ScriptingEngine.md). Type `help` to get a list of all possible commands.

## Listing Variables and Types ##

The `list` command allows you to list variables and types. To list all global variables starting with `init_`, for example, you can execute:

```
>>> list variables init_*
     ID  Base          Type name               Name                      Size  Source              
---------------------------------------------------------------------------------------------------
  172d6  Struct        struct signal_struct    init_signals               960  ...ernel/init_task.c
  172eb  Struct        struct sighand_struct   init_sighand              2088  ...ernel/init_task.c
  1785b  Union         union thread_union      init_thread_union         8192  ...ernel/init_task.c
  17871  Struct        struct task_struct      init_task                 1800  ...ernel/init_task.c
  2d0ef  Struct        struct uts_namespace    init_uts_ns                396  init/version.c      
 1e32aa  Pointer       struct xsave_struct *   init_xstate_buf              8  ...86/kernel/xsave.c
 33be99  Struct        atomic_t                init_deasserted              4  .../kernel/smpboot.c
 5c65a2  Struct        struct task_group       init_task_group            128  kernel/sched.c      
 6a3e08  Struct        struct user_namespace   init_user_ns              2096  kernel/user.c       
 6f5cb4  Struct        struct pid              init_struct_pid             80  kernel/pid.c        
 6f5cca  Struct        struct pid_namespace    init_pid_ns               2112  kernel/pid.c        
 768659  Struct        struct nsproxy          init_nsproxy                48  kernel/nsproxy.c    
 7ae216  Struct        struct thread_group...  init_tgcred                 48  kernel/cred.c       
 7aeae5  Struct        struct cred             init_cred                  144  kernel/cred.c       
 7c24cc  Struct        struct group_info       init_groups                144  kernel/groups.c     
 96bd0a  Struct        struct css_set          init_css_set               128  kernel/cgroup.c     
 96bd1f  Struct        struct cg_cgroup_link   init_css_set_link           48  kernel/cgroup.c     
 d7a055  Struct        struct mm_struct        init_mm                    880  mm/init-mm.c        
 f707cb  Struct        struct files_struct     init_files                 704  fs/file.c           
100727e  Struct        struct fs_struct        init_fs                     48  fs/fs_struct.c      
1425e93  Struct        struct ipc_namespace    init_ipc_ns                320  ipc/msgutil.c       
273d1db  Bool8         bool                    init_cpufreq_transit...      1  ...cpufreq/cpufreq.c
292774d  Struct        struct net              init_net                  2064  ...e/net_namespace.c
  8f00c  Array         pgd_t[?]                init_level4_pgt              8  arch/x86/xen/mmu.c  
 22ac0c  Union         union irq_stack_union   init_per_cpu__irq_st...  16384  ...rnel/cpu/common.c
---------------------------------------------------------------------------------------------------
Total variables: 25
>>> _
```

## Details about Variables and Types ##

You can show details about some variable or type with using the `show` command:

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
>>> _
```

## Executing Scripts ##

The `script` command executes JavaScript code. If you have installed the `insight-scripts` package, you find some example code in the directory `/usr/share/insight/examples/`:

```
>>> script /usr/share/insight/examples/modules.js 
Module [Args]        Used by
loop
snd_pcm
snd_timer            snd_pcm
parport_pc
snd                  snd_pcm, snd_timer
soundcore            snd
processor
i2c_piix4
parport              parport_pc
snd_page_alloc       snd_pcm
psmouse
evdev
button
serio_raw
i2c_core             i2c_piix4
pcspkr
ext3
jbd                  ext3
mbcache              ext3
ide_cd_mod
cdrom                ide_cd_mod
ide_gd_mod
ata_generic
ata_piix
libata               ata_generic, ata_piix
scsi_mod             libata
piix
8139too
thermal
floppy
ide_core             ide_cd_mod, ide_gd_mod, piix
thermal_sys          processor, thermal
8139cp
mii                  8139too, 8139cp
>>> _
```

## More Information and Examples ##

For more examples on how to use InSight, continue reading about the [shell](InSightShell.md) and the [scripting engine](ScriptingEngine.md) in this wiki.