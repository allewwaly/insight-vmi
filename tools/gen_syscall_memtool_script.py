#!/usr/bin/env python
#
# created on Tue 24 May 2011 01:20:09 PM CEST  by diekmann
#
#
# parse a 64bit linux kernel's source code to extract signatures for all syscalls
# output will be in tmp in cwd
# Though this code looks hacky, many checks are executed on the extracted symbols

import re
from pprint import pprint
import datetime

# TODO
#     |
#     |
#   \ | /
#    \|/
#     *
# change this!
KERNEL_SRC_DIR="/local_diekmann/build/victim_kernel/kernel/linux-2.6-2.6.32/"



# the file where Macros like __SYS_read are specified
SYS_xxx_DIR="arch/x86/include/asm/unistd_64.h"

# thef ile where the syscall signatures are specified
SYS_SIGNATURE_DIR="include/linux/syscalls.h"

# search #define __NR_read                               0
sys_line_regex = re.compile(r"""^#define\s+__NR_([a-z_0-9]+)[ \t]+(\d{1,3})\s*$""")


# an argument to a C function
C_fn_argument = r"""
(
	(?:const\s)??\s* (?:struct\s)?\s* (?:union\s)?\s* (?:unsigned\s)?\s* [a-z_0-9]+\s*	# argument type
)
(
	(?:
		(?:__user)?\s*\*?\s*	# __user and * prefix
	){0,2}				# there can be multiple __user and * prefixes
)
(
	[a-z_0-9]*		# argument name
)
"""

#search sysCall Signature like
#	asmlinkage long sys_clock_nanosleep(clockid_t which_clock, int flags,
#                                 const struct timespec __user *rqtp,
#                                 struct timespec __user *rmtp);
sys_signature_regex = re.compile(r"""
^
asmlinkage
\s+
[a-z]+		# return value
\s+
(sys_[a-z_0-9]+)	# name
\(
	(
		void | 
		(
			%s
			\s*
			(,\s* %s \s*)*
		)
	)
\);
\s*
$
""" % (C_fn_argument, C_fn_argument), re.VERBOSE | re.MULTILINE)

sysCalls = dict()

__allSysCalls = dict()

sysCallsByNR = dict()

if __name__ == "__main__":
	# read the sysCall numbers
	sys_nr_f = open(KERNEL_SRC_DIR+SYS_xxx_DIR, "r")
	for line in sys_nr_f:
		m = re.match(sys_line_regex, line)
		if(m):
			sys_name = m.group(1)
			sys_nr = m.group(2)
			
			# test for __SYSCALL(__NR_read, sys_read) in the next line
			line = sys_nr_f.next()
			
			#remove numbers
			sys_name_stripped_numbers = re.sub(r"([0-9]{0,2})", "", sys_name)
			
			pattern = """
			^__SYSCALL\(__NR_%s,\s
			# the following line represnets the function name for the sysCall
			# normally the name is sys_<NAME>, however there are some exceptions
			( (sys|stub)_(new)?%s(64)? | sys_ni_syscall | sys_sysctl | sys_%s )
			\)
			(\s/\*.*\*/\s)? # accept comments at end of line
			$""" % (sys_name, sys_name_stripped_numbers, sys_name)
			m = re.match(pattern, line, re.VERBOSE)
			if not m:
				print("expected: __SYSCALL(__NR_%s, sys_%s)" % (sys_name, sys_name_stripped_numbers) )
				print("got: %s" % line)
				raise BaseException("error prasing %s", SYS_xxx_DIR)
			
			sys_real_name = m.group(1)
			#print "%s %s %s" % (sys_name, sys_nr, sys_real_name)
			sysCalls["sys_"+sys_name] = {"sys_real_name": sys_real_name, "nr": sys_nr, "signature": None}
	sys_nr_f.close()
	
	#read sysCall signature
	sys_sig_f = open(KERNEL_SRC_DIR+SYS_SIGNATURE_DIR, "r")
	try:
		for line in sys_sig_f:
			while not re.match(r""".*;.*""", line, re.DOTALL):
				line += sys_sig_f.next()
			#print "parsing\"%s\" with %s" % (line, sys_signature_regex)
		
			# changed from match to search
			m = re.search(sys_signature_regex, line)
			#print(".")
			if m :
				#print m
				#print m.group(0)
				#print m.group(1)
				signature = re.sub(r"\s+", " ", m.group(2)) # remove annoying \n\t
				#print m.group(2)
				#print m.group(3)
				#print m.group(4)
				#print m.group(5)
				sys_real_name = m.group(1)
				if sys_real_name not in sysCalls:
					#print "%s has no number!" % sys_real_name
					pass
				else:
					sysCalls[sys_real_name]["signature"] = signature
				
				__allSysCalls[sys_real_name] = {"sig": signature, "matched_name": sys_real_name}
	except(StopIteration):
		print "last line (should not contain a syscall definition): \"%s\"" % line
		print("done parsing, checking ...")
	
	
	sys_sig_f.close()
	
	
	#pprint(sysCalls)
	
	#pprint(sysCallsByNR)
	
	for (k,v) in sysCalls.iteritems():
		if v["signature"] is None:
			print("No Signature for %s" %k)
	
	for (k,v) in sysCalls.iteritems():
		sysCallsByNR[int(v["nr"])] = {"sys_name": k, "sys_real_name": v["sys_real_name"], "signature": v["signature"]}
	
	
	#check if there is a syscall for every number
	i=0
	for k in sysCallsByNR.iterkeys():
		if k != i:
			print("No Syscall for NR %s" % i)
			i=k
		i += 1
	print("found %s syscalls" % (i-1))
	
	
	# extract signature,
	# create arg0 to argN entry containing a parsed version of the function arguments
	sig_regex = re.compile("^%s$" % C_fn_argument ,re.VERBOSE)
	for k in sysCallsByNR.iterkeys():
		if sysCallsByNR[k]["signature"] != None:
			sig_all = sysCallsByNR[k]["signature"].split(',')
			i = 0
			#print sig_all
			argc = 1
			for sig in sig_all:
				sig=sig.strip()
				m = re.match(sig_regex, sig)
				if not m:
					print("""Cannot parse signature for "%s": "%s" : "%s" """ % (k, sysCallsByNR[k]["signature"], sig))
					raise BaseException("fail")
			
				#print m
				#print m.group(0)
				argument_type = m.group(1).strip()
				user_ptr = m.group(2).strip()
				is_user = "__user" in user_ptr
				is_ptr = "*" in user_ptr
				argument_name = m.group(3).strip()
				#print argument_type
				#print user_ptr
				#print argument_name
				sysCallsByNR[k]["arg"+str(i)] = {"type": argument_type, "user_ptr": user_ptr, "name": argument_name,
						 "isUser": is_user, "isPtr":is_ptr}
				sysCallsByNR[k]["argc"] = argc
				argc += 1
				i += 1
	
	#check if there is an argument which is a pointer but not user (should not exist)
	#check argc
	for (k,v) in sysCallsByNR.iteritems():
		if v["signature"] != None:
			
			if v["arg0"]["isPtr"] and not v["arg0"]["isUser"]:
				print("pointer to not user: %s" % v)
			
			argc = 1
			for arg in ["arg1", "arg2", "arg3", "arg4", "arg5", "arg6"]:
				if arg in v:
					argc += 1
					if v[arg]["isPtr"] and not v[arg]["isUser"]:
						print("pointer to not user: %s" % v)
					if arg == "arg6":
						print("%s has 6 arguments" % v)
			
			if argc != v["argc"]:
				print("%s has wrong argc (%s != %s)" % (v, argc, v["argc"]))
			
			
	
	#pprint(__allSysCalls)
	out = open("./tmp", "w")
	
	out.write("// created by gen_syscall_memtool_script.py\n")
	out.write("// can be found in diekmann's git\n")
	out.write("// "+str(datetime.date.today())+"\n\n")
	
	out.write("// python compatibility\nNone = null\nFalse = false\nTrue = true\n\n")
	
	out.write("SysCalls = ")
	pprint(sysCallsByNR, out)
