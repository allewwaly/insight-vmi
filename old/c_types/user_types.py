# -*- coding: utf-8 -*-
from c_types import *

class String(Array):
    """
    represents a String in memory
    
    should be initialised with a Char-Array or Pointer to Char
    e.g.
	s = String(Array(type_of('unsigned char')))
	s = String(Pointer(type_of('char')))

    s will be a volatile type that cannot be referenced by other types
    unless s.register() is called
    """
    def __init__(self, typ):
	self.type_list = typ.type_list
	self.base = typ.base
	self.bound = typ.bound if isinstance(typ, Array) else None
	#self.register()
	
    def takeover(self, typ):
	"replaces all occurances of typ in the global type list with this entry"
	self.id = typ.id
	self.type_list[id] = self
	
    def value(self, loc, depth=0):
	base = self.type_list[self.base]
	return base.get_value(loc, 10) # 10 == MEMORY_TYPE_NULLTERMINATED_STRING
	
	#out = ""
	#i = 0
	#while 1:
	  #typ, val = base.value(loc + base.size*i, depth+1)
	  #if type(val) != int or val > 255 or val == 0 or val < -128: break
	  #out += chr((val+256)%256)
	  #i += 1
	#return out

class NullTerminatedArray(Array):
    """
    Implements an Array type that is terminated by a NULL value
    either a NULL-Pointer if the elements are pointers,
    or 0 if the elements are other basic values
    
    The class provides only a custom iterator.
    High-level functions such as __len__ will not produce correct
    results, as the output is not dependent on the type, but on the
    type and memory location.
    """
    def __iter__(self, loc, depth=MAX_DEPTH):
	base = self.get_base()
	i = 0
	while 1:
	  member, member_loc = self.__getitem__(i, loc, depth)
	  
	  if isinstance(base, Pointer):
	    if base.get_pointer_address(member_loc) == 0:
	      break
	  elif isinstance(base, BasicType) and base.value(member_loc) == 0:
	    break
	    
	  yield member, member_loc
	  i += 1

class KernelDoubleLinkedList(Struct):
    "Implements the linked lists which the kernel uses using preprocessor macros"
    
    parent = None
    offset = 0
    entries = {'next': 0, 'prev': 8}

    def __init__(self, struct, member, offset=None, name=None):
	"""
	initialises the linked list entry
	member is a Member instance for the list_head entry that is to be replaced
	if no offset is specified, member.offset will be used
	the name is entirely optional
	"""
	self.type_list = struct.type_list
	self._parent   = struct.id
	self.offset    = offset if offset is not None else member.offset
	self.name      = "list_head(%s)" % struct.name # "list_head(%s)" % struct.get_name()
    def takeover(self, member):
	"replaces all occurances of member in the global type list with this entry"
	if member.offset != self.offset:
	  raise Exception("cannot take over a foreign type!")
	self.id = member.id
	self.type_list[self.id] = self
    def parent(self, loc=None):
	if loc == 0: #NullPointerException
	  return Pointer(self.type_list[self._parent]), 0
	if not loc is None:
            return self.type_list[self._parent], loc - self.offset
	return self.type_list[self._parent]
    def get_pointer_value(self, loc, offset, image=0):
	if loc == 0:  raise NullPointerException(repr(self))
	ptr = resolve_pointer(loc + offset, image)
	# if we have a NULL pointer here then it is very likely
	# that we are at the beginning or end of a hlist_struct-like-list
	# or that there is no such list item, therefore throw a special 
	# EndOfListException
	if ptr == 0: raise EndOfListException(repr(self))
	#if ptr == 0: raise NullPointerException(repr(self))
	#print >>sys.stderr, "%x, %x" % (ptr, loc)
	return ptr
   
    def __getitem__(self, item, loc=None,  next_offset=None, image=0):
        if loc is None: return self.parent()
        if next_offset == None:
            next_offset = 0
        return self.parent(self.get_pointer_value(loc, self.entries[item], image) + next_offset)
    
    def __iter__(self, loc=None):
	for name,offset in self.entries.iteritems():
	  if loc is None:
	    yield self.parent()
	  else:
	    yield self.parent(self.get_pointer_value(loc,offset))

    def memcmp(self, loc, loc1, comparator, sympath=""):
	    if loc != loc1:
		    return False
	    next_offset = 0
	    try:	
		    if loc is None:
			    next_tuple = self.parent()
		    else:
			    next_tuple = self.parent(self.get_pointer_value(loc, self.entries["next"], 0) + next_offset)
		    if loc1 is None:
			    next1_tuple = self.parent()
		    else:
			    next1_tuple = self.parent(self.get_pointer_value(loc1, self.entries["next"], 1) + next_offset)
            except EndOfListException, e:
		    return True
	    # TODO: ignore lists, since they cause many problems ...
	    #       we should do the following:
	    # 	     - if we access some list in a struct struct.list
	    #	     - then take the next element of struct.list.next, but
	    #        - when comparing that keep in mind, that we have to 
	    #        - continue with the .list.next element ... struct.list.next.list.next ...
	    # TODO: seems to run in a big infinite loop when comparing the modules list for 
	    #       example
	    return True
#	    if self.get_name() != "list" or self.parent().get_name() != "modules":
#		    return True
	    comparator.enqueue_diff(sympath + ".next", next_tuple[0], next_tuple[1], next1_tuple[1])

    def revmap(self, loc, comparator, sympath=""):
	    comparator.just_add_rev(sympath + "." + self.get_name(), self, loc, self.get_size())

    def stringy(self, depth=0):
	return "\n".join(["\t%s → %s" % (name, self[name].__str__(depth+1).replace("\n", "\n\t"))
			  for name in self.entries])
    def value(self, loc, depth=MAX_DEPTH):
	# override this, since Struct.value will use the types name, which we suppress
	out = {}
	for key in self.entries:
	  member, loc = self.__getitem__(key, loc)
	  out[key] = member.value(loc, MAX_DEPTH-1)
	return out

    def __repr__(self):
	return "<%s instance '%s' offset %d>" % (self.__class__, self.get_name(), self.offset)

