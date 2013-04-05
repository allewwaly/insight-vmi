
// Provides isArray() function to test if an object is an array.
if (!Object.prototype.hasOwnProperty("isArray")) {
    Object.defineProperty(Object.prototype, "isArray", {
        value : function(){ return Array.prototype.isPrototypeOf(this); },
        configurable : false }
    );
}

// Provides isArray() function to test if an object is a string.
if (!Object.prototype.hasOwnProperty("isString")) {
    Object.defineProperty(Object.prototype, "isString", {
        value : function(){ return String.prototype.isPrototypeOf(this); },
        configurable : false }
    );
}
