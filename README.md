# split-appended-dtb
Tool to split a kernel image with appended dtbs into separated kernel and dtb files.

```
# Compile:
   gcc split-appended-dtb.c -o split-appended-dtb

```

# Usage:
   ./split-appended-dtb Image-dtb

```

# Output:
   kernel -> standalone kernel image
   dtbdump_*.dtb -> device tree blobs

```
