import * as shell from "mshell"

shell.println("Hello, world!")

let testObj = xx
testObj.a = 42;
testObj.b = 123;
testObj.c = "hello";
shell.println(JSON.stringify(testObj));
// Test add1 method
let sum = testObj.add1(10, 20);
shell.println("add1 result: " + sum);

// Test add2 method
let result = testObj.add2("test", "string");
shell.println("add2 result: " + result);