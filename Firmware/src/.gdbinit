file ../build/ch.elf
#target remote localhost:3333
target remote 192.168.56.1:3333
break main
#monitor program ../build/wookiectrl.elf
monitor verify
monitor reset halt
