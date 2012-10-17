
// Provides isArray() function to test if an object is an array.
Object.defineProperty(Object.prototype, "isArray", {
    value : function(){ return Array.prototype.isPrototypeOf(this); },
    configurable : false }
);

// Provides isArray() function to test if an object is a string.
Object.defineProperty(Object.prototype, "isString", {
    value : function(){ return String.prototype.isPrototypeOf(this); },
    configurable : false }
);
