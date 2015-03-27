InSight is a tool for [bridging the semantic gap](About#Background.md) in the field of virtual machine introspection (VMI) and digital forensics. It operates either on [memory dump files](InSightShell#Memory_Files.md) or in conjunction with any hypervisor that provides read access to the physical memory of a guest VM. InSight is written in C++ based on the Qt libraries and features [interactive analysis of kernel objects](InSightShell.md) as well as a [JavaScript engine](ScriptingEngine.md) for automation of repeating inspection tasks.

<img src='https://insight-vmi.googlecode.com/svn/wiki/images/tum-logo-pantone.png' align='right' />
This tool is developed by <a href='http://www.sec.in.tum.de/christian-schneider/'>Christian Schneider</a> and other contributors as part of <a href='http://www.sec.in.tum.de/leveraging-virtualization-techniques-for-system-security/'>their research</a> at the <a href='http://portal.mytum.de/welcome/?set_language=en'>Technische Universität München</a>, Munich, Germany. Together with his colleagues at the <a href='http://www.sec.in.tum.de/'>Chair for IT Security</a>, his interest lies in the field of virtual machine introspection and how this can be used in novel ways to improve current intrusion detection methods.

## Features ##

InSight runs on Linux and provides the following functionality:

  * analysis of physical **memory dumps** stored on disk as well as
  * analysis of **live** physical memory of a **virtual machine** (if the hypervisor provides access to the guest's memory)
  * support for **32-bit** and **64-bit** addressing schemes with and without PAE
  * re-creation of **kernel objects** for the Linux kernel
  * automatic **de-referencing of pointers** to futher objects
  * application of **type casts** and **pointer arithmetic** as the kernel would do
  * an **[interactive shell](InSightShell.md)** for manual analysis of kernel objects
  * a **[JavaScript engine](ScriptingEngine.md)** for automated analysis of repeating or complex tasks

## Example ##

The following example illustrates how the [JavaScript engine](ScriptingEngine.md) can be used to interact with the kernel objects of an analyzed system in an easy and intuitive way. It starts by requesting an instance of the global variable `init_task`. This variable is of type `struct task_struct` and represents the head to list of process control blocks in the Linux kernel. The example goes on by printing various information about the members of that structure, followed by in iteration over all processes in the system and printing their process ID as well as the command being run.

<table>
<tr>
<td>
<pre><code><br>
// Get instance of global variable "init_task"<br>
var init = new Instance("init_task");<br>
<br>
// Iterate over all members of "init_task"<br>
var members = init.Members();<br>
for (var i = 0; i &lt; members.length; ++i) {<br>
var offset = init.MemberOffset(members[i].Name());<br>
print ((i+1) +<br>
". 0x" + offset.toString(16) + " " +<br>
members[i].Name() + ": " +<br>
members[i].TypeName() +<br>
" @ 0x" + members[i].Address());<br>
}<br>
<br>
// Print a list of running processes<br>
var proc = init;<br>
do {<br>
print(proc.pid + "  " + proc.comm);<br>
proc = proc.tasks.next;<br>
} while (proc.Address() != init.Address());</code></pre>
</td>
<td width='20'>
</td>
<td>
<a href='https://insight-vmi.googlecode.com/svn/wiki/images/kernel_graph.png'><img src='https://insight-vmi.googlecode.com/svn/wiki/images/kernel_graph.png' title='Kernel Object Graph' align='right' height='296' width='400' alt='Kernel Object Graph' /></a>
</td>
</tr>
</table>


## Further Reading ##

The following pages are a recommended reading with more information about InSight, how to set it up and use it.

<table width='80%'>
<tr>
<td valign='top'>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_about.png' /></td>
<td width='8'></td>
<td>
<h3><a href='About.md'>About</a></h3>

Learn more about what InSight is, how it works and what features it currently provides.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_installation.png' /></td>
<td width='8'></td>
<td>
<h3><a href='Installation.md'>Installation</a></h3>

This page describes how to compile InSight from sources, optionally create Debian packages, install it, and set it up.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_running_insight.png' /></td>
<td width='8'></td>
<td>
<h3><a href='RunningInSight.md'>Running InSight</a></h3>

After InSight has been installed, it has to be started. This page explains how InSight is run from command line, how the <code>insight</code> front-end and the <code>insightd</code> daemon work together and which command line parameters they offer.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_sample-data.png' /></td>
<td width='8'></td>
<td>
<h3><a href='UsingSampleMemorySnapshots.md'>Sample Memory Snapshots</a></h3>

In order to try out InSight without having to generate and parse the debugging symbols for a virtual machine yourself, we provide some ready-to-use sample data.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_debug_symbols.png' /></td>
<td width='8'></td>
<td>
<h3><a href='DebugSymbols.md'>Debugging Symbols</a></h3>

Before any analysis can be performed on your own guest, InSight needs to parse the debugging symbols of the inspected operating system. This page shows how the debugging symbols can be obtained and how to process them with InSight.<br>
</td>
</tr>
</table>
</td>
<td width='30'>
</td>
<td valign='top'>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_shell.png' /></td>
<td width='8'></td>
<td>
<h3><a href='InSightShell.md'>The InSight Shell</a></h3>

The InSight shell is a powerful interface to process debugging symbols, load memory files, inspect data types and variables, execute scripts, amongst other things. This page describes the basic usage of the shell and how to perform first analysis tasks.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_script.png' /></td>
<td width='8'></td>
<td>
<h3><a href='ScriptingEngine.md'>The Scripting Engine</a></h3>

InSight comes with a JavaScript engine that allows to interact with kernel objects by means of JavaScript objects. This provides a very flexible and intuitive way to write analysis scripts for automation of complex analysis tasks.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_roadmap.png' /></td>
<td width='8'></td>
<td>
<h3><a href='RoadMap.md'>Road Map</a></h3>

InSight is a project in active development. The road map provides information about enhancements to InSight we are currently working on or planning to implement in the future.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_mailinglists.png' /></td>
<td width='8'></td>
<td>
<h3><a href='MailingLists.md'>Get in Contact</a></h3>

The InSight project provides several mailing lists to get in contact with other users and the developers.<br>
</td>
</tr>
</table>
</td>
</tr>
</table>

If you cannot find the information that you are looking for, you might want to try [searching the wiki pages](https://code.google.com/p/insight-vmi/w/list).