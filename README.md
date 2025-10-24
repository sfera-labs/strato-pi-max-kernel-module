# Strato Pi Max driver kernel module

Raspberry Pi OS (Debian) Kernel module for [Strato Pi Max](https://www.sferalabs.cc/strato-pi-max/).

It gives access to all Strato Pi Max functionalities and configuration options via sysfs virtual files.

## Compile and Install

*For installation on Ubuntu [read this](https://github.com/sfera-labs/knowledge-base/blob/main/raspberrypi/kernel-modules-ubuntu.md).*

Make sure your system is updated:

    sudo apt update
    sudo apt upgrade

If you are using a **32-bit** OS, add to `/boot/firmware/config.txt` (`/boot/config.txt` in older versions) the following line: [[why?](https://github.com/raspberrypi/firmware/issues/1795)]

    arm_64bit=0
    
Reboot:

    sudo reboot

After reboot, install git and the kernel headers:
 
     sudo apt install git linux-headers-$(uname -r)

Clone this repo:

    git clone --depth 1 https://github.com/sfera-labs/strato-pi-max-kernel-module.git

Make and install:

    cd strato-pi-max-kernel-module
    make clean
    make
    sudo make install
    
Compile the Device Tree overlay (replace `DTS_FILE` with `stratopimax-cm4.dts` or `stratopimax-cm5.dts` depending on the used CM version):

    dtc -@ -Hepapr -I dts -O dtb -o stratopimax.dtbo DTS_FILE

Install the overlay:

    sudo cp stratopimax.dtbo /boot/overlays/

To load the driver at boot, add to `/boot/firmware/config.txt` (`/boot/config.txt` in older versions) the following line:

    dtoverlay=stratopimax

If using Strato Pi Max **XL** with **CM5** and need access to the secondary SD, add the following line to `config.txt` too:

    dtoverlay=sdio-pi5

Optionally, to access the sysfs interface without superuser privileges, create a new group "stratopimax" and set it as the module owner group by adding a **udev** rule:

    sudo groupadd stratopimax
    sudo cp 99-stratopimax.rules /etc/udev/rules.d/

and add your user to the group, e.g., for user "pi":

    sudo usermod -a -G stratopimax pi

Reboot:

    sudo reboot


## Usage

After installation, you will find all the available devices under the `/sys/class/stratopimax/` path.

The following paragraphs list all the devices (directories) and properties (files) coresponding to Strato Pi Max's and its expansion boards' features. 

You can write to and/or read these files to configure, monitor and control your Strato Pi Max. The kernel module will take care of performing the corresponding GPIO or I2C operations.

Properties with the `_config` suffix represent configuration parameters that are saved and retained across power cycles.    
Configuration parameters marked with the `CR` attribute (see the <a href="#attributes">attributes table</a> below) take effect immediately when set. Conversely, parameters with the `C` attribute store the value to be applied during the next *power-up* by copying it to the corresponding "runtime" property (same name without the `_config` suffix).

> [!NOTE]  
> A *power-up* occurs when:
>
> - the unit is powered on (main power supply connected);
> - a power cycle is explicitly initiated via `power/down_enabled`; or
> - a power cycle is initiated by another process configured to do so (e.g. watchdog timeout expiration).
>
> A software reboot of the compute module does **not** correspond to a power-up. 

This enables configuring settings for the next power cycles, whether after a planned or abrupt shutdown. For example, it allows switching the boot SD card while the compute module is off or having a short watchdog timeout during application runtime, which resets to a longer duration after a power cycle to ensure reliable boot and restart.

<a name="attributes"></a>

All properties' attributes are summarized here:

|Attribute|Description|
|:--:|-----------|
|`R`|Readable|
|`W`|Writable|
|`RC`|Readable. Cleared when read|
|`WF`|Writable only when expansion board is off|
|`C`|Configuration value, persisted and retained across power cycles, copied to "runtime" counterpart on power-up|
|`CR`|Configuration value, persisted and retained across power cycles, effective immediately|


#### System - `/sys/class/stratopimax/system/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td>fw_version</td>
            <td>Firmware version</td>
            <td>
                <code>R</code>
            </td>
            <td><i>M</i>.<i>m</i></td>
            <td>Major (<i>M</i>) and minor (<i>m</i>) version</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>config</td>
            <td>Configuration commands</td>
            <td>
                <code>W</code>
            </td>
            <td>R</td>
            <td>Restore the factory configuration</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>sys_errs</td>
            <td>
                System errors flags<br/><br/>
                These flags are set as soon as an error occurs and are reset only upon reading.<br/>
                Upon error, operations are repeated, therefore these flags only represent an issue if persistent.
            </td>
            <td>
                <code>RC</code>
            </td>
            <td><i>MLKJxHGFxDCBA</i></td>
            <td>
                Bitmap (0/1) sequence.<br/>
                <i>A</i>: system setup error<br/>
                <i>B</i>: RP2040 error<br/>
                <i>C</i>: RP2040 reset occurred<br/>
                <i>D</i>: configuration loading/saving error<br/>
                <i>F</i>: RP2040 I2C master communication error<br/>
                <i>G</i>: RP2040 I2C slave communication error<br/>
                <i>H</i>: RP2040 SPI communication error<br/>
                <i>J</i>: USB ports fault<br/>
                <i>K</i>: I/O expanders communication error (see ioexp_errs)<br/>
                <i>L</i>: accelerometer communication error<br/>
                <i>M</i>: UPS fault<br/>
                <i>x</i>: reserved<br/>
            </td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>ioexp_errs</td>
            <td>
                I/O expanders communication errors flags<br/><br/>
                These flags are set as soon as an error occurs and are reset only upon reading.<br/>
                Upon error, operations are repeated, therefore these flags only represent an issue if persistent.
            </td>
            <td>
                <code>RC</code>
            </td>
            <td><i>GFEDCBA</i></td>
            <td>
                Bitmap (0/1) sequence.<br/>
                <i>A</i>: I/O expander BU17<br/>
                <i>B</i>: I/O expander BU21<br/>
                <i>C</i>: I/O expander BU22<br/>
                <i>D</i>: I/O expander on expansion board in slot 1<br/>
                <i>E</i>: I/O expander on expansion board in slot 2<br/>
                <i>F</i>: I/O expander on expansion board in slot 3<br/>
                <i>G</i>: I/O expander on expansion board in slot 4<br/>
            </td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Power cycle - `/sys/class/stratopimax/power/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>down_enabled</td>
            <td rowspan=2>Delayed power cycle enabling</td>
            <td>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>1</td>
            <td>Enabled. Once enabled cannot be interrupted</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>down_delay_config</td>
            <td>Shutdown delay from enabling</td>
            <td>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>1 ... 65535</td>
            <td>Value in seconds. Default: 60</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>off_time_config</td>
            <td>Power-off duration</td>
            <td>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>1 ... 65535</td>
            <td>Value in seconds. Default: 5</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>up_delay_config</td>
            <td>Power-up delay configuration. Defines the waiting period before powering on the compute module, ensuring main power remains stable for the specified duration before power on</td>
            <td>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0 ... 65535</td>
            <td>Value in seconds. Default: 0</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>up_backup_config</td>
            <td rowspan=2>Configuration for enabling power-up when a power cycle occurs while main power is not available (requires UPS expansion board)</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Power-up only when main power is restored (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Power-up even if running on backup power source (UPS battery)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>sd_switch_config</td>
            <td rowspan=2>SDA/SDB switch configuration on power cycle</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>pcie_switch_config</td>
            <td rowspan=2>PCIE on/off toggle configuration on power cycle</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Watchdog - `/sys/class/stratopimax/watchdog/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>enabled</td>
            <td rowspan=2>Watchdog enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>enabled_config</td>
            <td rowspan=2>Watchdog enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>timeout</td>
            <td>Watchdog timeout</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>1 ... 65535</td>
            <td>Value in seconds. Default: 60</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>timeout_config</td>
            <td>Watchdog timeout configuration</td>
            <td>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>60 ... 65535</td>
            <td>Value in seconds. Default: 60</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>down_delay_config</td>
            <td>Automatic power cycle delay configuration when watchdog timeout expires</td>
            <td>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0 ... 65535</td>
            <td>Value in seconds. Default: 60</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=3>sd_switch_config</td>
            <td rowspan=3>SDA/SDB switch configuration on watchdog reset</td>
            <td rowspan=3>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Switching occurs upon each watchdog reset</td>
        </tr>
        <tr>
            <td>2 ... 65535</td>
            <td>Number of consecutive resets (with no heartbeat received in between) before switching occurs. Requires <code>watchdog/enabled_config</code> set to <code>1</code></td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=3>pcie_switch_config</td>
            <td rowspan=3>PCIE on/off toggle configuration on watchdog reset</td>
            <td rowspan=3>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Toggling occurs upon each watchdog reset</td>
        </tr>
        <tr>
            <td>2 ... 65535</td>
            <td>Number of consecutive resets (with no heartbeat received in between) before toggling occurs. Requires <code>watchdog/enabled_config</code> set to <code>1</code></td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>heartbeat</td>
            <td>Watchdog heartbeat update</td>
            <td>
                <code>W</code>
            </td>
            <td>1</td>
            <td>Update heartbeat</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>expired</td>
            <td rowspan=2>Watchdog timeout state</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Not expired</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Expired</td>
        </tr>
    </tbody>
</table>

#### Button - `/sys/class/stratopimax/button/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>status</td>
            <td rowspan=2>Button state</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Not pressed</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Pressed</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>count</td>
            <td>Button presses counter</td>
            <td>
                <code>R</code>
            </td>
            <td>0 ... 255</td>
            <td>Rolls back to 0 after 255</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Buzzer - `/sys/class/stratopimax/buzzer/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=4>beep</td>
            <td rowspan=4>Buzzer beep control</td>
            <td rowspan=4>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Off</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Steady on</td>
        </tr>
        <tr>
            <td><i>T_ON</i> <i>T_OFF</i></td>
            <td>Continuos beep <i>T_ON</i> milliseconds on, <i>T_OFF</i> milliseconds off</td>
        </tr>
        <tr>
            <td><i>T_ON</i> <i>T_OFF</i> <i>REPS</i></td>
            <td>Beep <i>REPS</i> times <i>T_ON</i> milliseconds on, <i>T_OFF</i> milliseconds off</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>tone</td>
            <td>Buzzer beep tone</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0 ... 65535</td>
            <td>Value in Hz</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### LEDs - `/sys/class/stratopimax/led/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=4>red</td>
            <td rowspan=4>Red LED control</td>
            <td rowspan=4>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Off</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Steady on</td>
        </tr>
        <tr>
            <td><i>T_ON</i> <i>T_OFF</i></td>
            <td>Continuos blink <i>T_ON</i> milliseconds on, <i>T_OFF</i> milliseconds off</td>
        </tr>
        <tr>
            <td><i>T_ON</i> <i>T_OFF</i> <i>REPS</i></td>
            <td>Blink <i>REPS</i> times <i>T_ON</i> milliseconds on, <i>T_OFF</i> milliseconds off</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=4>green</td>
            <td rowspan=4>Green LED control</td>
            <td rowspan=4>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Off</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Steady on</td>
        </tr>
        <tr>
            <td><i>T_ON</i> <i>T_OFF</i></td>
            <td>Continuos blink <i>T_ON</i> milliseconds on, <i>T_OFF</i> milliseconds off</td>
        </tr>
        <tr>
            <td><i>T_ON</i> <i>T_OFF</i> <i>REPS</i></td>
            <td>Blink <i>REPS</i> times <i>T_ON</i> milliseconds on, <i>T_OFF</i> milliseconds off</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### USB - `/sys/class/stratopimax/usb/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>usb1_enabled</td>
            <td rowspan=2>USB1 enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>usb1_enabled_config</td>
            <td rowspan=2>USB1 enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>usb1_err</td>
            <td rowspan=2>USB1 fault</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>OK</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Fault</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>usb2_enabled</td>
            <td rowspan=2>USB2 enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>usb2_enabled_config</td>
            <td rowspan=2>USB2 enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>usb2_err</td>
            <td rowspan=2>USB2 fault</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>OK</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Fault</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### SD Cards Switch - `/sys/class/stratopimax/sd/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>sd_main_enabled</td>
            <td rowspan=2>Main SD interface enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>sd_main_enabled_config</td>
            <td rowspan=2>Main SD interface enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>sd_sec_enabled</td>
            <td rowspan=2>Secondary SD interface enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>sd_sec_enabled_config</td>
            <td rowspan=2>Secondary SD interface enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>sd_main_routing</td>
            <td rowspan=2>Main SD routing control</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>A</td>
            <td>Main SD interface routed to SDA, secondary SD interface to SDB</td>
        </tr>
        <tr>
            <td>B</td>
            <td>Main SD interface routed to SDB, secondary SD interface to SDA</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>sd_main_routing_config</td>
            <td rowspan=2>Main SD routing configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>A</td>
            <td>Main SD interface routed to SDA, secondary SD interface to SDB (default)</td>
        </tr>
        <tr>
            <td>B</td>
            <td>Main SD interface routed to SDB, secondary SD interface to SDA</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### PCIE interface - `/sys/class/stratopimax/pcie/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>enabled</td>
            <td rowspan=2>PCIE enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>enabled_config</td>
            <td rowspan=2>PCIE enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Power Supply Input - `/sys/class/stratopimax/power_in/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td>mon_v</td>
            <td>Power supply voltage monitor</td>
            <td>
                <code>R</code>
            </td>
            <td><i>V</i></td>
            <td>Value in mV</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>mon_i</td>
            <td>Power supply current drain monitor</td>
            <td>
                <code>R</code>
            </td>
            <td><i>V</i></td>
            <td>Value in mA</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Fan - `/sys/class/stratopimax/fan/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td>temp</td>
            <td>Fan controller measured temperature</td>
            <td>
                <code>R</code>
            </td>
            <td><i>T</i></td>
            <td>Value in °C/100</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>temp_on</td>
            <td>Temperature threshold for fan activation</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>-12800 ... 12750</td>
            <td>Value in °C/100, 0.5°C resolution</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>temp_off</td>
            <td>Temperature threshold for fan deactivation</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>-12800 ... 12750</td>
            <td>Value in °C/100, 0.5°C resolution</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Accelerometer - `/sys/class/stratopimax/accelerometer/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td>accel_x</td>
            <td>X-axis acceleration</td>
            <td>
                <code>R</code>
            </td>
            <td>-32768 ... 32767</td>
            <td>Raw value (resolution: 14-bit, full scale: ±2 g)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>accel_y</td>
            <td>Y-axis acceleration</td>
            <td>
                <code>R</code>
            </td>
            <td>-32768 ... 32767</td>
            <td>Raw value (resolution: 14-bit, full scale: ±2 g)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>accel_z</td>
            <td>Z-axis acceleration</td>
            <td>
                <code>R</code>
            </td>
            <td>-32768 ... 32767</td>
            <td>Raw value (resolution: 14-bit, full scale: ±2 g)</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Secure Element - `/sys/class/stratopimax/sec_elem/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td>serial_num</td>
            <td>Secure element serial number</td>
            <td>
                <code>R</code>
            </td>
            <td><i>HHHHHHHHHHHHHHHHHH</i></td>
            <td>HEX value</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

---

### Expansion Boards

#### Expansion Boards configuration - `/sys/class/stratopimax/exp_boards/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>s<i>N</i>_enabled</td>
            <td rowspan=2>Slot <i>N</i> enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>s<i>N</i>_enabled_config</td>
            <td rowspan=2>Slot <i>N</i> enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=8>s<i>N</i>_type</td>
            <td rowspan=8>Slot <i>N</i> expansion board type</td>
            <td rowspan=8>
                <code>R</code>
            </td>
            <td>1</td>
            <td>Empty slot</td>
        </tr>
        <tr>
            <td>2</td>
            <td>Uninterruptible Power Supply</td>
        </tr>
        <tr>
            <td>3</td>
            <td>CAN and dual RS-485</td>
        </tr>
        <tr>
            <td>4</td>
            <td>RS-232 and RS-485</td>
        </tr>
        <tr>
            <td>5</td>
            <td>Industrial digital I/O</td>
        </tr>
        <tr>
            <td>9</td>
            <td>Industrial analog inputs</td>
        </tr>
        <tr>
            <td>11</td>
            <td>Industrial analog outputs</td>
        </tr>
        <tr>
            <td>12</td>
            <td>Quad RS-422/RS-485</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

Below are the devices corresponding to each expansion board typology.

For expansion boards that can be installed on multiple slots, devices names have a `_s<n>` suffix denoting their slot number (1 to 4).

---

### Uninterruptible Power Supply (UPS) Expansion Board

#### UPS - `/sys/class/stratopimax/ups/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>enabled</td>
            <td rowspan=2>UPS enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>enabled_config</td>
            <td rowspan=2>UPS enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>battery_v_config</td>
            <td rowspan=2>UPS battery voltage configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>12000</td>
            <td>12 V battery (default)</td>
        </tr>
        <tr>
            <td>24000</td>
            <td>24 V battery</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>battery_capacity_config</td>
            <td>UPS battery capacity configuration</td>
            <td>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>100 ... 60000</td>
            <td>Value in mAh. Default: 800</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>battery_i_max</td>
            <td rowspan=2>UPS battery maximum charging current while at maximum voltage</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Value automatically derived from capacity (default)</td>
        </tr>
        <tr>
            <td>1 ... 65535</td>
            <td>Value in mA. The automatically derived limit still applies if lower than this value</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>battery_i_max_config</td>
            <td rowspan=2>UPS battery maximum charging current while at maximum voltage configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Value automatically derived from capacity (default)</td>
        </tr>
        <tr>
            <td>1 ... 65535</td>
            <td>Value in mA. The automatically derived limit still applies if lower than this value</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>down_delay_config</td>
            <td rowspan=2>Timeout configuration for automatic power cycle enabling upon main power failure</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1 ... 65535</td>
            <td>Value in seconds</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>backup</td>
            <td rowspan=2>Power source state</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Running on main power</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Running on backup battery</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=10>status</td>
            <td rowspan=10>UPS status</td>
            <td rowspan=10>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Idle</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Detecting battery</td>
        </tr>
        <tr>
            <td>2</td>
            <td>Battery disconnected</td>
        </tr>
        <tr>
            <td>4</td>
            <td>Charging battery</td>
        </tr>
        <tr>
            <td>5</td>
            <td>Battery charged</td>
        </tr>
        <tr>
            <td>6</td>
            <td>Running on battery</td>
        </tr>
        <tr>
            <td>8</td>
            <td>Battery over-voltage error</td>
        </tr>
        <tr>
            <td>9</td>
            <td>Battery under-voltage error</td>
        </tr>
        <tr>
            <td>10</td>
            <td>Charger damaged</td>
        </tr>
        <tr>
            <td>11</td>
            <td>Unstable</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>charger_mon_v</td>
            <td>Battery charger output voltage monitor</td>
            <td>
                <code>R</code>
            </td>
            <td>0 ... 65535</td>
            <td>Value in mV</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>charger_mon_i</td>
            <td>Battery charger output current drain monitor</td>
            <td>
                <code>R</code>
            </td>
            <td>0 ... 65535</td>
            <td>Value in mA</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Power Supply Output - `/sys/class/stratopimax/power_out/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>vso_enabled</td>
            <td rowspan=2>VSO power supply output control</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>vso_enabled_config</td>
            <td rowspan=2>VSO power supply output configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

---

### SuperCaps UPS Expansion Board

FW ver. &ge; 3.37

#### UPS - `/sys/class/stratopimax/ups/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>enabled</td>
            <td rowspan=2>UPS enabling</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>enabled_config</td>
            <td rowspan=2>UPS enabling configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>C</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>backup</td>
            <td rowspan=2>Power source state</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Running on main power</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Running on backup power</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=7>status</td>
            <td rowspan=7>UPS status</td>
            <td rowspan=7>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Idle</td>
        </tr>
        <tr>
            <td>4</td>
            <td>Charging / Not ready</td>
        </tr>
        <tr>
            <td>5</td>
            <td>Charged / Ready</td>
        </tr>
        <tr>
            <td>6</td>
            <td>Running on backup power</td>
        </tr>
        <tr>
            <td>7</td>
            <td>Running on backup power, below ready threshold</td>
        </tr>
        <tr>
            <td>10</td>
            <td>Fault</td>
        </tr>
        <tr>
            <td>11</td>
            <td>Unstable</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

---

### Industrial Digital I/O Expansion Board

#### Digital inputs - `/sys/class/stratopimax/digital_in_s<n>/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>in<i>N</i>_wb_config</td>
            <td rowspan=2>Input <i>N</i> Wire-break detection configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=9>in<i>N</i>_filter_config</td>
            <td rowspan=9>Input <i>N</i> filter configuration</td>
            <td rowspan=9>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>50 µs delay (default)</i></td>
        </tr>
        <tr>
            <td>1</td>
            <td>100 µs delay</i></td>
        </tr>
        <tr>
            <td>2</td>
            <td>400 µs delay</i></td>
        </tr>
        <tr>
            <td>3</td>
            <td>800 µs delay</i></td>
        </tr>
        <tr>
            <td>4</td>
            <td>1.6 ms delay</i></td>
        </tr>
        <tr>
            <td>5</td>
            <td>3.2 ms delay</i></td>
        </tr>
        <tr>
            <td>6</td>
            <td>12.8 ms delay</i></td>
        </tr>
        <tr>
            <td>7</td>
            <td>20 ms delay</i></td>
        </tr>
        <tr>
            <td>8</td>
            <td>Filter bypassed</i></td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>in<i>N</i></td>
            <td rowspan=2>Input <i>N</i> state</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Low</td>
        </tr>
        <tr>
            <td>1</td>
            <td>High</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>inputs</td>
            <td>Inputs state combined</td>
            <td>
                <code>R</code>
            </td>
            <td><i>SSSSSSS</i></td>
            <td>Concatenation of all 7 inputs states</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>in<i>N</i>_wb</td>
            <td rowspan=2>Input <i>N</i> wire-break</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Not detected (write to clear)</td>
        </tr>
        <tr>
            <td>
                <code>R</code>
            </td>
            <td>1</td>
            <td>Detected (set until cleared)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>inputs_wb</td>
            <td rowspan=2>Inputs wire-break combined</td>
            <td>
                <code>R</code>
            </td>
            <td><i>SSSSSSS</i></td>
            <td>Concatenation of all 7 inputs wire-break states</td>
        </tr>
        <tr>
            <td>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Clear</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>in<i>N</i>_cnt<br/>(FW ver. &ge; 3.24)</td>
            <td>Input <i>N</i> state change counter</td>
            <td>
                <code>R</code>
            </td>
            <td>0 ... 65535</td>
            <td>Increases every time input <i>N</i> changes state. Rolls back to 0 after 65535</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>alarm_t1</td>
            <td rowspan=2>Inputs temperature alarm 1</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Not active (write to clear)</td>
        </tr>
        <tr>
            <td>
                <code>R</code>
            </td>
            <td>1</td>
            <td>Active (set until cleared)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>alarm_t2</td>
            <td rowspan=2>Inputs temperature alarm 2</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Not active (write to clear)</td>
        </tr>
        <tr>
            <td>
                <code>R</code>
            </td>
            <td>1</td>
            <td>Active (set until cleared)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>over_temp</td>
            <td rowspan=2>Inputs thermal shutdown</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Not active (write to clear)</td>
        </tr>
        <tr>
            <td>
                <code>R</code>
            </td>
            <td>1</td>
            <td>Active (set until cleared)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>fault</td>
            <td rowspan=2>Inputs general fault</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Not active</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Active</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

#### Digital outputs - `/sys/class/stratopimax/digital_out_s<n>/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>out<i>N</i>_pp_config</td>
            <td rowspan=2>Output <i>N</i> high-side/push-pull mode configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>High-side mode (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Push-pull mode</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>out<i>N</i>_ol_config</td>
            <td rowspan=2>Output <i>N</i> open-load detection configuration (only for high-side mode)</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>join_l_config</td>
            <td rowspan=2>Outputs 1-2 (L side terminal block) join configuration (only for high-side mode)</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Outputs 1-2 joined. Configurations and status for output 1 apply to pair</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>join_h_config</td>
            <td rowspan=2>Outputs 4-5 and 6-7 (H side terminal block) join configuration (only for high-side mode)</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Outputs 4-5 and 6-7 joined. Configurations and status for outputs 4 and 6 apply to respective pairs</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>watchdog_config</td>
            <td rowspan=2>Outputs watchdog configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=3>watchdog_timeout_config</td>
            <td rowspan=3>Outputs watchdog timeout configuration</td>
            <td rowspan=3>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>0.9 s (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>0.45 s</td>
        </tr>
        <tr>
            <td>2</td>
            <td>0.15 s</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>out<i>N</i></td>
            <td rowspan=2>Output <i>N</i> state</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Low</td>
        </tr>
        <tr>
            <td>1</td>
            <td>High</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>outputs</td>
            <td>Outputs state combined</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td><i>SSSSSSS</i></td>
            <td>Concatenation of all 7 outputs states</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>out<i>N</i>_ol</td>
            <td rowspan=2>Output <i>N</i> open-load</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Not detected (write to clear)</td>
        </tr>
        <tr>
            <td>
                <code>R</code>
            </td>
            <td>1</td>
            <td>Detected (set until cleared)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>outputs_ol</td>
            <td rowspan=2>Outputs open-load combined</td>
            <td>
                <code>R</code>
            </td>
            <td><i>SSSSSSS</i></td>
            <td>Concatenation of all 7 outputs open-load states</td>
        </tr>
        <tr>
            <td>
                <code>W</code>
            </td>
            <td>0</td>
            <td>clear</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>out<i>N</i>_ov</td>
            <td rowspan=2>Output <i>N</i> over-voltage</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Not detected (write to clear)</td>
        </tr>
        <tr>
            <td>
                <code>R</code>
            </td>
            <td>1</td>
            <td>Detected (set until cleared)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>outputs_ol</td>
            <td rowspan=2>Outputs over-voltage combined</td>
            <td>
                <code>R</code>
            </td>
            <td><i>SSSSSSS</i></td>
            <td>Concatenation of all 7 outputs over-voltage states</td>
        </tr>
        <tr>
            <td>
                <code>W</code>
            </td>
            <td>0</td>
            <td>clear</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>out<i>N</i>_ot</td>
            <td rowspan=2>Output <i>N</i> over-temperature</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Not detected (write to clear)</td>
        </tr>
        <tr>
            <td>
                <code>R</code>
            </td>
            <td>1</td>
            <td>Detected (set until cleared)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>outputs_ot</td>
            <td rowspan=2>Outputs over-temperature combined</td>
            <td>
                <code>R</code>
            </td>
            <td><i>SSSSSSS</i></td>
            <td>Concatenation of all 7 outputs over-temperature states</td>
        </tr>
        <tr>
            <td>
                <code>W</code>
            </td>
            <td>0</td>
            <td>clear</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>out<i>N</i>_ov_lock</td>
            <td rowspan=2>Output <i>N</i> over-voltage protection lock</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Not active</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Active</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>outputs_ov_lock</td>
            <td>Outputs over-voltage protection lock combined</td>
            <td>
                <code>R</code>
            </td>
            <td><i>SSSSSSS</i></td>
            <td>Concatenation of all 7 outputs over-voltage protection lock states</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>out<i>N</i>_ot_lock</td>
            <td rowspan=2>Output <i>N</i> over-temperature protection lock</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Not active</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Active</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>outputs_ot_lock</td>
            <td>Outputs over-temperature protection lock combined</td>
            <td>
                <code>R</code>
            </td>
            <td><i>SSSSSSS</i></td>
            <td>Concatenation of all 7 outputs over-temperature protection lock states</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

---

### RS-485 Expansion Boards

#### RS-485 config - `/sys/class/stratopimax/rs485_s<n>/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>echo_config</td>
            <td rowspan=2>RS-485 local echo configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

---

### Analog Inputs Expansion Board

FW ver. &ge; 3.26

#### Analog inputs - `/sys/class/stratopimax/analog_in_s<n>/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>av<i>N</i>_enabled_config</td>
            <td rowspan=2>Analog voltage input <i>N</i> enabled configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>av<i>N</i>_bipolar_config</td>
            <td rowspan=2>Analog voltage input <i>N</i> bipolar configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Unipolar: range 0 ... 20 V (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Bipolar: range -20 ... 20 V</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>av<i>N</i>_differential_config</td>
            <td rowspan=2>Analog voltage input <i>N</i> (1 or 3) differential configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Single-ended: AGND is the voltage reference (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Differential: voltage difference between input <i>N</i> and <i>N</i>+1 </td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>ai<i>N</i>_enabled_config</td>
            <td rowspan=2>Analog current input <i>N</i> enabled configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>av_filter_config</td>
            <td>Analog voltage inputs filter configuration</td>
            <td>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>
                Filter configuration register (FILTCON<i>x</i>) value used for voltage inputs.<br/>
                Refer to the AD4112 datasheet for details.<br/>
                Default: 1294 (sinc5 + sinc1 filter, 100.2 SPS data rate)
            </td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>ai_filter_config</td>
            <td>Analog current inputs filter configuration</td>
            <td>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>
                Filter configuration register (FILTCON<i>x</i>) value used for current inputs.<br/>
                Refer to the AD4112 datasheet for details.<br/>
                Default: 1294 (sinc5 + sinc1 filter, 100.2 SPS data rate)
            </td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>at<i>N</i>_enabled_config</td>
            <td rowspan=2>Temperature probe input <i>N</i> enabled configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>at<i>N</i>_pt1000_config</td>
            <td rowspan=2>Temperature probe <i>N</i> type configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Pt100 (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Pt1000</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=4>av<i>N</i></td>
            <td rowspan=4>Analog voltage input <i>N</i> value</td>
            <td rowspan=4>
                <code>R</code>
            </td>
            <td><i>V</i></td>
            <td>Value in mV/100</td>
        </tr>
        <tr>
            <td>8388607</td>
            <td>Overrange value</td>
        </tr>
        <tr>
            <td>-8388607</td>
            <td>Underrange value</td>
        </tr>
        <tr>
            <td>-8388608</td>
            <td>Error</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=4>ai<i>N</i></td>
            <td rowspan=4>Analog current input <i>N</i> value</td>
            <td rowspan=4>
                <code>R</code>
            </td>
            <td><i>V</i></td>
            <td>Value in µA</td>
        </tr>
        <tr>
            <td>8388607</td>
            <td>Overrange value</td>
        </tr>
        <tr>
            <td>-8388607</td>
            <td>Underrange value</td>
        </tr>
        <tr>
            <td>-8388608</td>
            <td>Error</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=4>at<i>N</i></td>
            <td rowspan=4>Temperature probe <i>N</i> value</td>
            <td rowspan=4>
                <code>R</code>
            </td>
            <td><i>V</i></td>
            <td>Value in °C/100</td>
        </tr>
        <tr>
            <td>8388607</td>
            <td>Overrange value</td>
        </tr>
        <tr>
            <td>-8388607</td>
            <td>Underrange value</td>
        </tr>
        <tr>
            <td>-8388608</td>
            <td>Error</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>av<i>N</i>_sps</td>
            <td>Analog voltage input <i>N</i> effective SPS</td>
            <td>
                <code>R</code>
            </td>
            <td><i>V</i></td>
            <td>Number of samples read from this channel, updated every second</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>ai<i>N</i>_sps</td>
            <td>Analog current input <i>N</i> effective SPS</td>
            <td>
                <code>R</code>
            </td>
            <td><i>V</i></td>
            <td>Number of samples read from this channel, updated every second</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>v5_fault</td>
            <td rowspan=2>5V output fault</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>OK</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Fault</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>


---

### Analog Outputs Expansion Board

FW ver. &ge; 3.21

#### Analog outputs - `/sys/class/stratopimax/analog_out_s<n>/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>ao<i>N</i>_mode_config</td>
            <td rowspan=2>Analog ouput <i>N</i> mode configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>WF</code>
                <code>CR</code>
            </td>
            <td>V</td>
            <td>Voltage (default)</td>
        </tr>
        <tr>
            <td>I</td>
            <td>Current</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>ao<i>N</i></td>
            <td rowspan=2>Analog output <i>N</i> value</td>
            <td>
                <code>R</code>
                <code>W</code>
            </td>
            <td><i>V</i></td>
            <td>Value, in mV (voltage mode - range 0...10417) or µA (current mode - range 0...20833)</td>
        </tr>
        <tr>
            <td>
                <code>R</code>    
            </td>
            <td>65535</td>
            <td>Error</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>ao<i>N_errs</td>
            <td>Analog output <i>N</i> error flags</td>
            <td>
                <code>R</code>
            </td>
            <td><i>CLT</i></td>
            <td>Bitmap (0/1) sequence.<br/>
                <i>T</i>: over-temperature error<br/>
                <i>L</i>: load error<br/>
                <i>C</i>: common mode error</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>v5_fault</td>
            <td rowspan=2>5V output fault</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>OK</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Fault</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

---

### Quad RS-422/RS-485 Expansion Board

FW ver. &ge; 3.29

#### Config - `/sys/class/stratopimax/rs422_s<n>/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>ch<i>N</i>_half_duplex_config<br/>(1 &le; <i>N</i> &le; 4)</td>
            <td rowspan=2>Channel <i>N</i> half-duplex configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Full-duplex (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Half-duplex (RS-485 mode)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>ch<i>N</i>_echo_config<br/>(1 &le; <i>N</i> &le; 4)</td>
            <td rowspan=2>Channel <i>N</i> local echo configuration (half-duplex only)</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>ch<i>N</i>_termination_config<br/>(1 &le; <i>N</i> &le; 4)</td>
            <td rowspan=2>Channel <i>N</i> line termination resistors configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>ch<i>N</i>_slew_config<br/>(1 &le; <i>N</i> &le; 4)</td>
            <td rowspan=2>Channel <i>N</i> slew limit configuration</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

---

### M.2 LTE Module Expansion Boards

FW ver. &ge; 3.40

#### Module control - `/sys/class/stratopimax/lte_s<n>/`

<table>
    <thead>
        <tr>
            <th>File</th>
            <th>Description</th>
            <th><a href="#attributes">Attr</a></th>
            <th>Value</th>
            <th>Value description</th>
        </tr>
    </thead>
    <!-- ================= -->
    <tbody>
        <tr>
            <td rowspan=2>enabled</td>
            <td rowspan=2>Module power control</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Off</td>
        </tr>
        <tr>
            <td>1</td>
            <td>On (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>rf_enabled</td>
            <td rowspan=2>RF control</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled (airplane mode)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>gps_enabled</td>
            <td rowspan=2>GPS control</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled (default)</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>reset</td>
            <td rowspan=2>Module reset control</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>gpio5</td>
            <td rowspan=2>GPIO 5 output control</td>
            <td rowspan=2>
                <code>R</code>
                <code>W</code>
            </td>
            <td>0</td>
            <td>Low (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>High</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>gpio6</td>
            <td rowspan=2>GPIO 6 input state</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Low</td>
        </tr>
        <tr>
            <td>1</td>
            <td>High</td>
        </tr>
        <!-- ------------- -->
    </tbody>
</table>

