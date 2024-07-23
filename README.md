**bombtag** 
will wait for one or more programs to voluntarily exit. 
If necessary, it'll forcibly terminate them, 
first with SIGTERM and then with SIGKILL.

```
Usage: bombtag [options]
-t seconds     Timeout before SIGTERM (default 30)
-k seconds     After SIGTERM, timeout before SIGKILL (default 30)
-p pid         PID of the process that shall exit
-n name        Name of the process that shall exit
```

You can use `bombtag -n <name>` like a time-delayed replacement for `killall`.

### Setup

See `compile_in_docker.sh` and the `Dockerfile`.

Also, the entire source code is **128 lines of code**,
so feel free to just read it.
