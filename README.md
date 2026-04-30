# ps5-linux

**ps5-linux** leverages a patched HV vulnerability to transform your PS5 Phat console running **3.xx or 4.xx firmwares** into a highly capable Linux PC, unlocking its full hardware potential for desktop use. Powered by 8 CPU cores (16 threads) at **3.5 GHz** and a GPU at **2.23 GHz**, it provides enough performance to run Steam games and various emulators with impressive fluidity. It supports HDMI 4K60 video and audio output. Furthermore, it allows you to utilize an **M.2 SSD** as a dedicated Linux partition, as well as all USB ports on the console.

![Alt Text](logo.webp)

## PS5 firmware

*ps5-linux* is currently only supported on PS5 Phat on 3.xx and 4.xx firmwares.

- **3.00**, **3.10**, **3.20**, **3.21**, without M.2 support
- **4.00**, **4.02**, **4.03**, **4.50**, **4.51** with M.2 support

Support for 1.xx and 2.xx firmwares may be added in the future, but we will not prioritize this effort.

Support for 5.xx firmwares may be added in the future, but for those firmwares, Linux will run within the GameOS VM, thus it will have less features (still unknown what limitations there will be) and it may not perform as good.

If you want to update to a specific firmware, [download the correct PUP](https://darthsternie.net/ps5-firmwares/) and follow the [official guide](https://www.playstation.com/en-us/support/hardware/reinstall-playstation-system-software-safe-mode) to upgrade your PS5.

## Hardwares

To run *ps5-linux*, you need some required and optional hardwares:

- **Required**: USB drive with minimum 64GB (ideally external SSD) to install and run Linux.
- **Required**: USB Ethernet/WLAN adapter for internet access.
- **Required**: USB keyboard/mouse (dongles supported too).
- *Optional*: M.2 SSD compatible on PS5 (see [official guide](https://www.playstation.com/en-us/support/hardware/ps5-install-m2-ssd)) to run Linux from SSD.
- *Optional*: Bluetooth dongle to connect with PS5 DualSense controller.


## Configure PS5 settings

- **Required**: Enable Rest Mode features:
  - Go to `Settings` → `System` → `Power Saving` → `Features Available in Rest Mode` and set `Supply Power to USB Ports` to `Always`.
- **Required**: Disable HDMI Device Link:
  - Go to `Settings` → `HDMI` → `Enable HDMI Device Link`
- *Recommended*: Disable automatic updates:
  - Go to `Settings` → `System Software` → `System Software Update and Settings`
- *Recommended*: Disable automatic error reporting:
  - Go to `Settings` → `System Software` → `Report System Software Errors Automatically`

## Installation

### 1. Get a Linux image

#### Linux/macOS:

```bash
git clone https://github.com/ps5-linux/ps5-linux-image
cd ps5-linux-image
chmod +x ./build_image.sh
./build_image.sh --distro ubuntu2604
```

#### Windows (WSL2):

If WSL2 is not installed yet, run this in PowerShell or CMD as administrator, then restart:

```bash
wsl --install
```

Then open WSL and set up Docker:

```bash
sudo apt update
sudo apt install docker.io -y
sudo service docker start
sudo usermod -aG docker $USER
```

Restart WSL from PowerShell/CMD:

```bash
wsl --shutdown
```

Then clone and build:

```bash
cd ~/
git clone https://github.com/ps5-linux/ps5-linux-image
cd ps5-linux-image
chmod +x ./build_image.sh
./build_image.sh --distro ubuntu2604
```

The finished image is written to `output/ps5-ubuntu2604.img`.

### 2. Flash the image to USB
Minimum drive size: 64 GB. An external SSD is strongly recommended.

#### Linux/macOS:

```bash
# check drive name with lsblk / diskutil list
sudo dd if=output/ps5-ubuntu2604.img of=/dev/sdX bs=4M status=progress conv=fsync
```

#### Windows (Balena Etcher):

Download Balena Etcher, select the .img file, select your USB drive, click Flash.

#### Windows (WSL2 + usbipd):

Install usbipd in PowerShell as administrator:

```bash
winget install usbipd
```

Plug in your USB drive, list devices and find the busid of your drive:

```bash
usbipd list
```

Bind and attach it to WSL (replace 5-3 with your busid):

```bash
usbipd bind --busid 5-3
usbipd attach --busid 5-3 --wsl
```

Then flash from WSL:

```bash
lsblk  # confirm the drive appeared, e.g. /dev/sdb
sudo wipefs -a /dev/sdX
sudo dd if=output/ps5-ubuntu2604.img of=/dev/sdX bs=4M status=progress
```

### 3. Plug the USB drive into your PS5

The following USB ports are supported for booting:

- Front bottom Type-C port
- Rear Type-A ports

The front top Type-A port is USB 2.0 which is slower and thus not recommended.

### 4. Run the jailbreak exploit

1. Clone https://github.com/idlesauce/umtx2
2. Configure fakedns via `dns.conf` to point `manuals.playstation.net` to your PCs IP address
3. Run fake dns: `python fakedns.py -c dns.conf`
4. Run HTTPS server: `python host.py`
5. Go into PS5 advanced network settings and set primary DNS to your PCs IP address and leave secondary at `0.0.0.0`
6. Go to user manual in settings and accept untrusted certificate prompt, run.

#### 5. Send the payload
Either download [ps5-linux-loader.elf](https://github.com/ps5-linux/ps5-linux-loader/releases/), or install [ps5-payload-sdk](https://github.com/ps5-payload-dev/sdk) and compile it yourself:

```bash
git clone https://github.com/ps5-linux/ps5-linux-loader
cd ps5-linux-loader
make
```
## Compiling on ARM64 Linux

Install the x86-64 cross-compilation tools before:

```bash
sudo apt install gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu
```


Find your PS5 IP at `Settings → Network → View Connection Status`.

```bash
socat -t 99999999 - TCP:192.168.178.127:9021 < ps5-linux-loader.elf
```

If all is successful, the payload will automatically go into rest mode. Wait until the orange LED stops blinking and becomes static. Only then, press the power button again to boot your PS5 into Linux. If the boot is successful, **the LED should turn white**. If it boots back into PS5 OS, then it's because you pressed the power button too early. Or, you did not enable rest mode features as described above.

If the LED is white, but you still have a blackscreen then:

- Try different monitors or capture cards, ideally with different resolutions. Currently, some monitors have issues.
- Try setting `amdgpu.force_1080p=1` in `cmdline.txt` in the FAT32 partition of the USB drive.

If none of this helps, please report the issue in our [Discord server](https://discord.gg/PeMGVB7BAm) and provide your EDID information.


## First Boot

Configure your system and memorize your login password.

Then, there are certain settings and commands we recommend doing:

1. Disable screen saver, as it is currently buggy.

2. Possibly, you have to disable and reenable your Wired/WLAN connection to get internet connection.

3. Install Firefox:

   ```bash
   sudo snap install firefox
   sudo snap refresh mesa-2404 --channel=latest/edge
   ```

4. Clone our [ps5-linux-tools](https://github.com/ps5-linux/ps5-linux-tools):

   ```bash
   git clone https://github.com/ps5-linux/ps5-linux-tools
   ```

## M.2 installation

You can use a M.2 SSD exclusively for Linux (which means you cannot use it for PS5 game storage).

1. Attach the M.SSD and format it on your PS5.
2. Boot Linux on your PS5 and run these commands to initialize your M.2:

```bash
sudo apt install zlib1g-dev
cd ps5-linux-tools
gcc -o m2_init m2_init.c -lz
sudo ./m2_init
```

3. Reboot via `sudo reboot`. If your PS5 asks you to format your M.2 again, please report this issue to us in our [Discord server](https://discord.gg/PeMGVB7BAm) and provide your M.2 model and storage size.
4. Relaunch Linux on your PS5.
5. Copy the `ps5-ubuntu2604.img` image that you built during installation or rebuild it on your PS5. Then, install it onto your M.2:

```bash
cd ps5-linux-tools
chmod +x ./m2_install.sh
sudo ./m2_install.sh --install $PATH_TO_YOUR_IMG
```

Now, you can boot into Linux on your M.2 via:

```bash
cd ps5-linux-tools
chmod +x ./m2_exec.sh
sudo ./m2_exec.sh
```

In order to always boot Linux from your M.2, you can edit the label at `/boot/efi/cmdline.txt` from `root=LABEL=ubuntu2604` to `root=LABEL=ubuntu2604-m2`.

## Fan & boost control

We provide a simple tool that allows you to boost your CPU to 3500Mhz and GPU to 2230MHz as well as changing the fan curve:

```bash
cd ps5-linux-tools
gcc -o ps5_control ps5_control.c
sudo ./ps5_control --fan on
sudo ./ps5_control --boost on
```

Always turn on fan when your turn on boost, as this is what the official PS5 OS does.

## FAQ

- Q: Can I dual-boot Linux and PS5 OS?
  - A: No, this is a soft-mod. You need to re-run the exploit in order to boot into Linux.
- Q: Can I put Linux into standby and resume?
  - A: No, this is not supported. We may however add a shutdown feature that puts your PS5 into rest-mode allowing you to relaunch Linux when powering up again.

- Q: Can I continue using my PS5 if I install Linux?
  - A: Yes, the internal SSD is not modified
- Q: Can I use the PS5's NIC/WLAN module in Linux?
  - A: In theory yes, but someone needs to write or adapt drivers to use them.
- Q: Will higher >=6.xx firmwares be supported?
  - A: No.
- Q: Does the DualSense controller work?
  - A: Via a Bluetooth dongle. Built-in Bluetooth is not yet supported.
- Q: What resolutions and refresh rates are supported?
  - A: So far only 1080p, 1440p and 2160p at 60Hz. 120Hz or 30Hz may be added in the future.


## Tips and tricks

- You can adjust the kernel cmdline in `cmdline.txt` in the FAT32 partition.
- You can adjust the VRAM size in `vram.txt` in the FAT32 partition. By default, it uses 512MB (0x20000000) which enables [Dynamic VRAM allocation](https://elektricm.github.io/amd-bc250-docs/bios/flashing/#why-flash-the-bios).
- Monitor hotswap may work, but it will not change resolution automatically.

Many configurations, tips and tricks from the [AMD BC250 Documentation](https://elektricm.github.io/amd-bc250-docs/) also apply to PS5.

## Bugs

- screen save does not work properly
- hdmi audio output does not work on some monitors
- hdmi 1440p and 2160p video output does not work on some monitors

## Upstreamed changes

During this project, we have upstreamed some changes:

- [drm/amd: fix dcn 2.01 check](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/drivers/gpu/drm/amd/display/dc?id=39f44f54afa58661ecae9c27e15f5dbce2372892)
- [amd/addrlib: Add more GFX1013 GPUs](https://gitlab.freedesktop.org/mesa/mesa/-/commit/44bed00b8bbcb1825e2c920cf1a828efdc72b1f1)

## Discord

Join our [Discord server](https://discord.gg/PeMGVB7BAm) to celebrate Linux on PS5, receive help, learn tips & tricks, join development, or report issues.

## Credits

- [theflow](https://github.com/TheOfficialFloW): [ps5-linux-loader](https://github.com/ps5-linux/ps5-linux-loader), [ps5-linux-patches](https://github.com/ps5-linux/ps5-linux-patches), [ps5-linux-tools](https://github.com/ps5-linux/ps5-linux-tools)
- [c0w](https://github.com/c0w-ar): [ps5-linux-loader](https://github.com/ps5-linux/ps5-linux-loader)
- [resulknad](https://github.com/resulknad): [ps5-linux-image](https://github.com/ps5-linux/ps5-linux-image)
- [fail0verflow](https://github.com/fail0verflow): [prosperous](https://github.com/fail0verflow/prosperous)
- [flatz](github.com/flatz): [HV exploit](https://gist.github.com/flatz/620ddda6d64acca6d1c990dc3080ac0e)
- [cragson](https://github.com/cragson): [HV expoit implementation](https://github.com/cragson/ps5-hen)
- [john-tornblom](https://github.com/john-tornblom): [PS5 SDK](https://github.com/ps5-payload-dev/sdk)
- [echostretch](https://github.com/echostretch): Offsets and testing
- [kirathenotebook](https://github.com/kirathenotebook): Betatesting and readme contribution
- 15432: Tests on BC-250
