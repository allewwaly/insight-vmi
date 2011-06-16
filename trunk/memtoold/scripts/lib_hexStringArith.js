/**
 * add two hex strings a and b of arbitrary length
 * no hex prefix like 0x is expected
 */
function __hex_add(a, b){
	if(typeof(a) != typeof("str")) throw("a must be string!");
	if(typeof(b) != typeof("str")) throw("b must be string!");
	
	
	if(b.length > a.length){
		var tmp = a;
		a = b;
		b = tmp;
	}
	
	if(a.indexOf("0x") === 0){
		a = a.substr(2);
	}
	if(b.indexOf("0x") === 0){
		b = b.substr(2);
	}
	
	
	for(var i = 0; b.length < a.length; i++){
		b = "0"+b;
	} 
	if(a.length != b.length) throw("a and b must have same length, a:"+a+" b:"+b);
	
	//print(a.length)
	
	var result_hex = ""
	var carry = 0;
	for(var i = a.length-1; i >= 0 ; --i){
		var tmp_a = parseInt(a[i], 16);
		if(tmp_a.toString() == "NaN") throw("not a hex Number:"+a+" ,"+a[i]);
		var tmp_b = parseInt(b[i], 16);
		if(tmp_b.toString() == "NaN") throw("not a hex Number:"+b);
		
		var tmp_result = (tmp_a + tmp_b + carry).toString(16);
		if(tmp_result.length == 1){
			result_hex = tmp_result+result_hex;
			carry = 0;
		}else if(tmp_result.length == 2){
			result_hex = tmp_result[1]+result_hex;
			carry = parseInt(tmp_result[0], 16);
		}else throw("Arith Error: "+tmp_result+" tmp_a:"+tmp_a+" tmp_b:"+tmp_b+" carry:"+carry);
	}
	if(carry != 0){
		result_hex = carry.toString(16)+result_hex;
	}
	
	return result_hex;
	
}

POINTER_SIZE=8

//print(__hex_add("ffff","1234f"));
//print(__hex_add("f", "f"));


/**
 * unsigned integer to hex String
 * Instance is modified
 */
function __uintToHex(instance){
	var res_hex = "";
	for(var i = 0; i < instance.Size(); ++i){
		var addrLow = instance.toUInt8()
		addrLow = parseInt(addrLow).toString(16)
		res_hex = addrLow + res_hex
		if(addrLow.length == 1){
			res_hex = "0" + res_hex
		}
		instance.AddToAddress(1)
	}
	return res_hex;
}
