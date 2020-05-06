# Debugging the driver

## Prerequisites
 * [QEMU 2.12.0](https://www.qemu.org/download/)
 * [Virtme 0.0.3](https://github.com/amluto/virtme) also see nice [article](https://www.collabora.com/news-and-blog/blog/2018/09/18/virtme-the-kernel-developers-best-friend/) from Collabora
 * [Lua 5.2.4](https://www.lua.org/download.html)

## Running in a test environment
Debugging and development of kernel module may be an issue - crashes or resource leakage in kernelspace sometimes can be irreversible. To simplify host debugging I use qemu with virtme helper tools and couple of custom scripts:
 * scripts/emulate.sh - starts a vm with current host's kernel (so you need to build the driver using its headers) and passes to it USB host controller that is used to connect FL2000 to
 * scripts/startup.sh - loads kernel modules and dependenceis for debugging/testing (execution can be automated with virtme in future)

It seems that QEMU has some issues with isochronous transfers: when scheduling a series 16x3x1024 transfers for 1152000 bytes I saw only handful of threm actually transmitted. In order to enable isochronous transfers debug & development, I give whole USB Host Controller to VM and avoid using qemu-xhci model. Similar problem was discovered with USB Interrupt IN transfers - endpoint is always stalling during debug after couple transactions.

Make sure you have IOMMU enabled on kernel boot: `amd_iommu=on"` on AMD host or `intel_iommu=on` on Intel host. For distros with VFIO builtin (e.g. Ubuntu 20.04) you also need to add kernel boot arg; `vfio-pci.ids=$PCI_ID` where PCI_ID is identifier of your USB Host Controller that you want to debug with. In case if you do not have VFIO built into the kernel (e.g. Ubuntu 19.10), you need to start it before running emulation:<br>
```bash
modprobe vfio
modprobe vfio_iommu_type1
modprobe vfio_pci ids=$PCI_ID
modprobe vfio_virqfd
```

Examples above were tested with ASMedia Technology Inc. ASM1142 USB 3.1 Host Controller.

## Register & I2C programming
Sequnce of register configuration and writing is not documented well for either FL2000 or IT66121. In both cases code implementations are rather basic and it is very hard to simply copy-paste them into "clean" implementation. This is also true for DDC I2C communication for EDID processing or IT66121 modes configuration or frames streaming. On the other hand, Windows driver for FL2000DRM based dongle is fully available and seem to be working properly: connected display resolution recognized, Windows desktop can be seen, no artifacts present, etc. This is true for VGA and HDMI dongle versions, which means that programming for both FL2000 and IT66121 can be studied. This makes possible implementation of HW related stuff via reverse-engineering: dump USB bus interactions, parse them with Wireshark or similar tool, formalize logic and implement it.

Since there is lots of unknowns, it could be beneficial to create a flexible system that would allow userspace register programming without the need to rebuild the driver and restart the VM; when programming model and flow is clarified, implementation can be moved from userspace to the driver modules. With this approach, first implementation is done in userspace using simple Lua script and driver-exposed custom debugfs entries for accessing registers and I2C and interrupts status values ringbuffer, as well as sending frame buffer stream.

**Register programming** `/sys/kernel/debug/fl2000_regs`
- `reg_address`: specify register address to work with
- `reg_data`: read causes read from register, write causes write to register

**I2C access** `/sys/kernel/debug/fl2000_i2c`
- `i2c_address`: i2c device address to operate with
- `i2c_offset`: specify device register address to work with
- `i2c_value`: read causes i2c read, write causes i2c write

**Interrupt handling** `/sys/kernel/debug/fl2000_interrupt`
- `intr_status`: array of values of interrupt statuses since last reading

Following functions are already implemented in the driver code:
* interrupts status register processing
* automatic IT66121 client detection on I2C bus

*TODO* Add buffer for sending arbitrary frame without using DRM

## USB bus debug
Capture 10000 packets and store in temp folderof virtual machine
```bash
tcpdump -i usbmon2 -nn -w /tmp/usb.pcap -c 10000 &
```
then upload to host via `scp` (sshd must be runningand configured), e.g.
```bash
scp /tmp/usb.pcap klogg@10.0.2.2:/home/klogg/Downloads
```

## DRM implementation

### kmscube
Test with kmscube built with default settings (see https://gitlab.freedesktop.org/mesa/kmscube/)
```bash
kmscube -D /dev/dri/card0 -v 800x480-66
```

### mplayer
Default installation of mplayer and some demo media file (Google it)
```bash
mplayer -nolirc -nosound -vo fbdev2:/dev/fb0 -vf scale=800:-3 file_example_MP4_640_3MG.mp4
```
