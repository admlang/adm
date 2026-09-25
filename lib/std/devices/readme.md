Reference for the numbers in `v4l2.adm`:

https://docs.kernel.org/userspace-api/media/v4l/v4l2.html
https://docs.kernel.org/userspace-api/media/v4l/mmap.html
https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/videodev2.h

The sizes, offsets and request numbers are those of 64-bit Linux; a request
number embeds the size of the structure it carries, so another architecture
adds its own table beside this one.
