# ESP32_scripting
Running scripts on VM
Example:
```
string test[] {
  "A1 ", "B4 ", "C2 "
}

function print_all(string[] a) {
  for int i = 0; i < array.length(a); i = i + 1 {
    uart.write(a[i]);
  }
}

print_all(test);
```
Work in progress...
