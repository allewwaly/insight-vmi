# Getting Started #

The following pages cover the first steps that need to be performed in order to install InSight, obtain the debugging symbols, start it up, and perform you first analysis.

<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_installation.png' /></td>
<td width='8'></td>
<td>
<h2><a href='Installation.md'>Installation</a></h2>

The first part describes how to compile InSight from sources, optionally create Debian packages, install it, and set it up.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_running_insight.png' /></td>
<td width='8'></td>
<td>
<h2><a href='RunningInSight.md'>Running InSight</a></h2>

After InSight has been installed, it needs to be started. The second part explains how InSight is run from command line, how the <code>insight</code> front-end and the <code>insightd</code> daemon work together and which command line parameters they offer.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_sample-data.png' /></td>
<td width='8'></td>
<td>
<h2><a href='UsingSampleMemorySnapshots.md'>Quick Start with Sample Memory Snapshots</a></h2>

In order to try out InSight without having to generate and parse the debugging symbols for a virtual machine yourself, we provide some ready-to-use sample data.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_debug_symbols.png' /></td>
<td width='8'></td>
<td>
<h2><a href='DebugSymbols.md'>Debugging Symbols</a></h2>

Before any analysis can be performed on your own guest, InSight needs to parse the debugging symbols of the inspected operating system. This page shows how the debugging symbols can be obtained and how to process them with InSight.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_shell.png' /></td>
<td width='8'></td>
<td>
<h2><a href='InSightShell.md'>The InSight Shell</a></h2>

The InSight shell is a powerful interface to process debugging symbols, load memory files, inspect data types and variables, execute scripts, amongst other things. This part describes the basic usage of the shell and how to perform first analysis tasks.<br>
</td>
</tr>
</table>
<table>
<tr>
<td><img src='https://insight-vmi.googlecode.com/svn/wiki/images/pg_script.png' /></td>
<td width='8'></td>
<td>
<h2><a href='ScriptingEngine.md'>The Scripting Engine</a></h2>

InSight comes with a JavaScript engine that allows to interact with kernel objects by means of JavaScript objects. This provides a very flexible and intuitive way to write analysis scripts for automation of complex analysis tasks.<br>
</td>
</tr>
</table>