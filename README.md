modulecrcpatch
===============

This is a tool to patch kernel symbol crcs in a kernel module.
The usage is as follows:
```
modulecrcpatch [readfile] [writefile]
```

The tool will find all kernel symbols in [writefile]. It will search the corresponding symbols
in [readfile] and copy the crc from [readfile] to [writefile].

This is for example useful if you have a kernel module and want to use it in
different Android versions / firmwares (usually module_layout symbol crc is different).

You can compile the code with Android NDK for example, there is a pre-compiled binary in libs/armeabi.