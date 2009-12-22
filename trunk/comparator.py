# -*- coding: utf-8 -*-

from c_types import *
from threading import Thread, RLock, Lock
import signal

class ComparatorThread(Thread):
	def __init__(self, comparator):
		Thread.__init__(self)
		self.comparator = comparator
		self.compared = 0
		for sig in range(1, signal.NSIG):
			try:
		        	signal.signal(sig, signal.SIG_DFL)
	        	except RuntimeError, e:
				pass
		pass

	def run(self):
		tasks = self.comparator.fetch_tasks()
		while len(tasks) > 0:
			while len(tasks) > 0:
				sympath, type, loc, loc1 = tasks.pop(0)
#				print hex(loc), ": ", sympath, "t:", repr(type), ",lq: ", len(tasks), ",q: ", len(self.comparator.queue), ",s: ", len(self.comparator.seen)
				try:
					self.compared += 1
					r = type.memcmp(loc, loc1, self.comparator, sympath)
					if r != None and r == False:
						print "differing: ", sympath
				except UserspaceVirtualAddressException, e:
					self.comparator.finc()
					pass
				except NullPointerException, e:
					self.comparator.finc()
					pass
				except MemoryAccessException, e:
					self.comparator.finc()
					pass
#				except AttributeError, e:
#					# TODO: where does that come from?
#					print "attr error"
#					self.comparator.finc()
#					pass
			tasks = self.comparator.fetch_tasks()
		return

class RevmapThread(Thread):
	def __init__(self, comparator):
		Thread.__init__(self)
		self.comparator = comparator
		self.revmapped = 0
		for sig in range(1, signal.NSIG):
			try:
		        	signal.signal(sig, signal.SIG_DFL)
	        	except RuntimeError, e:
				pass
		pass

	def run(self):
		tasks = self.comparator.fetch_tasks()
		while len(tasks) > 0:
			while len(tasks) > 0:
				sympath, type, loc = tasks.pop(0)
#				print hex(loc), ": ", sympath, "lq: ", len(tasks), ",q: ", len(self.comparator.queue), ",s: ", len(self.comparator.seen)
				try:
					self.revmapped += 1
					type.revmap(loc, self.comparator, sympath)
				except UserspaceVirtualAddressException, e:
					self.comparator.finc()
					pass
				except NullPointerException, e:
					self.comparator.finc()
					pass
				except MemoryAccessException, e:
					self.comparator.finc()
					pass
			tasks = self.comparator.fetch_tasks()
		return



class Comparator():
	"""
	Manages an internal list of symbols to compare and then compares then
	"""
	def __init__(self):
		self.queue = []
		self.seen = {}
		self.seen_rev = set([])  
		self.revmap_set = []
		self.slock = Lock()
		self.qlock = Lock()
		self.flock = Lock()
		self.threads = []
		self.printc = 0
		self.faults = 0
		pass

	def finc(self):
		self.flock.acquire()
		self.faults += 1
		self.flock.release()
		
	def enqueue_diff(self, sympath, type, loc, loc1):
		"""
		Enqueue an item to the compare list
		"""
		self.slock.acquire()
		try:
			if self.seen[type] != None:
				if loc in self.seen[type]:
					self.slock.release()
					return
				else:
					self.seen[type].add(loc)
		except KeyError, e:
			self.seen[type] = set([loc])
		self.qlock.acquire()
		self.queue.append((sympath, type, loc, loc1))
		self.qlock.release()
		self.slock.release()
		return
	
	def just_add_rev(self, sympath, type, loc, size):
		"""
		Just add the symbol and location to the seen list
		"""
		self.slock.acquire()
		self.seen_rev.add(loc)
		self.revmap_set.append((loc, size, type))
		self.slock.release()
		return

	def enqueue_rev(self, sympath, type, loc, size):
		"""
		Enqueue an item to the reverse mapping jobs list
		"""
		self.slock.acquire()
		if loc in self.seen_rev:
			self.slock.release()
			return
		self.qlock.acquire()
		self.seen_rev.add(loc)
		self.revmap_set.append((loc, size, type))
		self.queue.append((sympath, type, loc))
		self.qlock.release()
		self.slock.release()
		return

	def revmap(self, num_threads=1):
		"""
		run reverse mapping calculation until queue is empty
		"""
#		print "Starting thread ...",
		for i in range(num_threads):
#			print i,
			cur = RevmapThread(self)
			self.threads.append(cur)
		for i in range(num_threads):
			self.threads[i].start()

#		print "\nWaiting for threads to finish ..."

		for i in range(num_threads):
			self.threads[i].join()
#			print "\nTerminated: ", i


#		print "\nTotal compared: ", len(self.seen)
#		for i in range(num_threads):
#			print "Thread ", i, " compared: ", self.threads[i].revmapped

#		print "faults: ", self.faults, " ", ((1.0 * self.faults) / len(self.seen)) * 100.0, "%%"
		# (revmap, faults)
		return (self.revmap_set, self.faults)


	def run(self, num_threads=1):
		"""
		run comparision until queue ist empty
		"""
#		print "Starting thread ...",
		for i in range(num_threads):
#			print i,
			cur = ComparatorThread(self)
			self.threads.append(cur)
		for i in range(num_threads):
			self.threads[i].start()

#		print "\nWaiting for threads to finish ..."

		for i in range(num_threads):
			self.threads[i].join()
#			print "\nTerminated: ", i


#		print "\nTotal compared: ", len(self.seen)
#		for i in range(num_threads):
#			print "Thread ", i, " compared: ", self.threads[i].compared

		print "faults: ", self.faults, " ", ((1.0 * self.faults) / len(self.seen)) * 100.0, "%%"
		# (total compared, faults)
		return (len(self.seen), self.faults)
	
	def fetch_tasks(self, count=100):
		#self.printc += 1
		#if self.printc >= 1:
#		print "q: %i, s: %i\r" % (len(self.queue), len(self.seen)),
		#	self.printc = 0
		self.qlock.acquire()
		x = self.queue[0:count]
		del self.queue[0:count]
		self.qlock.release()
		return x

	def print_queue(self):
		print self.queue

