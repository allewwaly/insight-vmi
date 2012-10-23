
include("lib_string.js");
include("lib_util.js");


var c_addr = 2+8;
var c_size = 8;
var c_name = 28;


function printSlubCaches()
{
    var item = new Instance("task_struct_cachep");

    if (!item.IsValid()) {
        throw "Could not find variable 'task_struct_cachep'";
	}

    var endAddr = item.Address();

    c_addr = 2 + item.SizeofPointer() * 2;
    var c_total = c_name + 2 +
                  c_size + 2 +
                  c_size + 2 +
                  c_size + 2 +
                  c_size + 2 +
                  c_size + 2 +
                  c_addr;

    // Print header
    print(lalign("name", c_name));
    print ("  ");
    print(ralign("size", c_size));
    print ("  ");
    print(ralign("objsize", c_size));
    print("  ");
    print(ralign("align", c_size));
//    print("  ");
    print(ralign("total_obj", c_size+2));
    print("  ");
    print(ralign("nr_slabs", c_size));
    print("  ");
    print(lalign("cache addr.", c_addr));
    println();
    println(hline(c_total));

    var cnt = 0;
    while (!item.IsNull()) {

        if (item.name != "NULL") {
            cnt++;
            print(lalign(item.name, c_name));
            print("  ");
            print(ralign(item.size, c_size));
            print("  ");
            print(ralign(item.objsize, c_size));
            print("  ");
            print(ralign(item.align, c_size));
            print("  ");
            print(ralign(item.local_node.total_objects.counter, c_size));
            print("  ");
            print(ralign(item.local_node.nr_slabs.counter, c_size));
            print("  ");
            print(lalign("0x" + item.Address(), c_size));
            println();
        }

		
        item = item.list.next;
        if (item.Address() == endAddr)
            break;
    }

    println(hline(c_total));
    println("Total:", cnt, "caches");
}

printSlubCaches();

