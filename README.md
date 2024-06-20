# Strato Pi Max driver kernel module

Raspberry Pi OS Kernel module for [Strato Pi Max](https://www.sferalabs.cc/products/strato-pi-max/).

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
    
After reboot, install git and the Raspberry Pi kernel headers:

    sudo apt install git raspberrypi-kernel-headers

Clone this repo:

    git clone --depth 1 https://github.com/sfera-labs/strato-pi-max-kernel-module.git

Make and install:

    cd strato-pi-max-kernel-module
    make clean
    make
    sudo make install
    
Compile the Device Tree and install it:

    dtc -@ -Hepapr -I dts -O dtb -o stratopimax.dtbo stratopimax.dts
    sudo cp stratopimax.dtbo /boot/overlays/

This overlay, amongst the different peripherals, configures the secondary SD interface to use the `sdhost` controller, so that `sdio` is available for WiFi on CM units equipped with it.

To load the driver at boot, add to `/boot/firmware/config.txt` (`/boot/config.txt` in older versions) the following line:

    dtoverlay=stratopimax

Optionally, to be able to use the `/sys/class/stratopimax/` files not as super user, create a new group "stratopimax" and set it as the module owner group by adding an udev rule:

    sudo groupadd stratopimax
    sudo cp 99-stratopimax.rules /etc/udev/rules.d/

and add your user to the group, e.g. for user "pi":

    sudo usermod -a -G stratopimax pi

Reboot:

    sudo reboot


## Usage

After loading the module, you will find all the available devices under the path `/sys/class/stratopimax/`.

The following paragraphs list all the devices (directories) and properties (files) coresponding to Strato Pi Max's and its expansion boards' features. 

You can write and/or read these files to configure, monitor and control your Strato Pi Max. The kernel module will take care of performing the corresponding GPIO or I2C operations.

Properties with the `_config` name suffix correspond to configuration parameters whose values are persisted and retained across power cycles.    
The configuration parameters with attribute `CR` (see the <a href="#attributes">attributes table</a> below) are effective immediately, while those marked with `C` only store the value which will be copied to their "runtime" counterpart (same property name without `_config` suffix) upon power-up.

> [!NOTE]  
> A *power-up* occurs when:
>
> - the unit is powered on (main power supply connected);
> - a power cycle is explicitly initiated via `power/down_enabled`; or
> - a power cycle is initiated by another process configured to do so (e.g. watchdog timeout expiration).
>
> A software reboot of the compute module does **not** correspond to a power-up. 

This allows us to have a different configuration for the next power-up phase, whether after a planned or abrupt power-off. For example, it enables switching the boot SD card while the compute module is off or setting a short watchdog timeout only while your application (which manages the watchdog heartbeat) is running; the timeout is then reset to a longer duration when a power cycle occurs, ensuring the compute module has enough time to boot and restart your application.

<a name="attributes"></a>

All properties' attributes are summarized here:

|Attribute|Description|
|:--:|-----------|
|`R`|Readable|
|`W`|Writable|
|`RC`|Readable. Cleared when read|
|`WF`|Writable only when expansion board is off|
|`C`|Configuration value, persisted and retained across power cycles, value copied to "runtime" counterpart on power cycle|
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
            <td>System errors flags</td>
            <td>
                <code>RC</code>
            </td>
            <td><i>MLKJxHGFxDCBA</i></td>
            <td>
                Bitmap (0/1) sequence.<br/>
                <i>A</i>: system setup failure<br/>
                <i>B</i>: RP2040 failure<br/>
                <i>C</i>: RP2040 reset occurred<br/>
                <i>D</i>: configuration loading/saving error<br/>
                <i>F</i>: RP2040 I2C master failure<br/>
                <i>G</i>: RP2040 I2C slave failure<br/>
                <i>H</i>: RP2040 SPI failure<br/>
                <i>J</i>: USB ports fault<br/>
                <i>K</i>: I/O expanders fault (see ioexp_errs)<br/>
                <i>L</i>: accelerometer fault<br/>
                <i>M</i>: UPS fault<br/>
            </td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td>ioexp_errs</td>
            <td>I/O expanders errors flags</td>
            <td>
                <code>RC</code>
            </td>
            <td><i>GFEDCBA</i></td>
            <td>
                Bitmap (0/1) sequence.<br/>
                <i>A</i>: I/O expander BU17 fault<br/>
                <i>B</i>: I/O expander BU21 fault<br/>
                <i>C</i>: I/O expander BU22 fault<br/>
                <i>D</i>: I/O expander on expansion board in slot 1 fault<br/>
                <i>E</i>: I/O expander on expansion board in slot 2 fault<br/>
                <i>F</i>: I/O expander on expansion board in slot 3 fault<br/>
                <i>G</i>: I/O expander on expansion board in slot 4 fault<br/>
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
            <td>Power-up delay configuration</td>
            <td>
                <code>R</code>
                <code>W</code>
                <code>CR</code>
            </td>
            <td>1 ... 65535</td>
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
            <td rowspan=4>s<i>N</i>_type</td>
            <td rowspan=4>Slot <i>N</i> expansion board type</td>
            <td rowspan=4>
                <code>R</code>
            </td>
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
            <td>Disabled (default)</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Enabled</td>
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
            <td rowspan=2>power_delay_config</td>
            <td rowspan=2>Automatic power cycle timeout configuration when main power source not available</td>
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
            <td rowspan=2>battery</td>
            <td rowspan=2>Power source state</td>
            <td rowspan=2>
                <code>R</code>
            </td>
            <td>0</td>
            <td>Running on main power</td>
        </tr>
        <tr>
            <td>1</td>
            <td>Running on battery</td>
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
            <td>clear</td>
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
            <td rowspan=2>Outputs 1-2 (L side) join configuration. Can be joined only in high-side mode. `out2_mode` will be ignored</td>
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
            <td>Outputs 1-2 joined</td>
        </tr>
        <!-- ------------- -->
        <tr>
            <td rowspan=2>join_h_config</td>
            <td rowspan=2>Outputs 4-5 and 6-7 (H side) join configuration. Can be joined only in high-side mode. `out5_mode` and `out7_mode` will be ignored</td>
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
            <td>Outputs 4-5 and 6-7 joined</td>
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

#### Digital inputs - `/sys/class/stratopimax/rs485_s<n>/`

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
