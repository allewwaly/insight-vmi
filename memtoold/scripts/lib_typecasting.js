/*
 * created by diekmann on Fri 27 May 2011
 *
 * functions for memorytool typecasting
 *
 *
 */
 

// changes the type of _inst_ to _type_
// @ return true on success, false otherwise
function __changeType(inst, type){
	try{
		inst.ChangeType(type)
		return true;
	}catch(e){
		return false;
	}
}

// try to change the type of _inst_ to _type_
// heuristically tries to make the typeChange succeed,
// e.g: if memtool doesn't know "const char", this function
// will also try "char" before giving up
// throws exception
function __tryChangeType(inst, type){
	try{
		// generate exception we want to rethrow if
		// we cannot find a suitable type
		inst.ChangeType(type) ;
		return;
	}catch(e){
		//print("-->"+type);
		if(type=="unsigned long"){
			if(__changeType(inst, "uint64_t")) return;
		}
		
		var foundMatchingType = false;
		var typeList = type.split(" ");
		for(i=0; i<typeList.length; i++){
			var testType = ""
			for(j=i; j<typeList.length; j++){
				testType += typeList[i] + " "
			}
			testType = testType.substr(0, testType.length-1);
			//print("trying \""+testType+"\"");
			if(!__changeType(inst, testType)){
				//print("fail");
			}else{
				foundMatchingType = true;
				break;
			}
		}
		if(!foundMatchingType){
			throw(e);
		}
	}
	
}

