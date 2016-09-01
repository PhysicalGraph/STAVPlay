# stavplay

Samsung Tizen NaCl player for low latency live streaming of RTSP streams.

## prerequisites

> Feel free to ignore anything related to `sectv-orsay` in the following links.

+ [Install the Tizen SDK](https://www.samsungdforum.com/TizenDevtools/SdkDownload)
+ [Add command line tools to path](https://developer.tizen.org/development/tools/native-tools/command-line-interface#nw_run)
+ [Creating an emulator](https://www.samsungdforum.com/TizenGuide/tizen2911/index.html)
+ [Getting a certificate](https://www.samsungdforum.com/TizenGuide/tizen3531/index.html)
+ [Installing certificate in the emulator](https://www.samsungdforum.com/TizenGuide/tizen3531/index.html#Permitting-Application-Installation)
+ [Setup the web development environment](https://github.com/Samsung/cordova-plugin-toast/wiki/Prepare-to-start)

## libraries

Download/update compiled libraries from S3:

`./update_libs.sh`

## cert

Create a cert in the IDE according to the instructions above. I named mine `smartthings` which is used in the following examples. For the command line tools to sign the wgt you must tell it where to find the `profiles.xml` file. This file is tpyically found in your IDE workspace directory.

```
alias tizen=~/tizen-sdk/tools/ide/bin/tizen
# add profiles.xml as global default
tizen cli-config -g default.profiles.path=~/workspace/.metadata/.plugins/org.tizen.common.sign/profiles.xml
# check to make sure your cert is listed
tizen security-profiles list
```

## build

The build can take several minutes. Set `V=1` for verbose output.

> You may need to refresh the project view to see newly built objects in the IDE.

`CERT=smartthings make`

## emulator

The emulator does not support most `sdb` commands. To run on the emulator you must use the IDE.

Import the project.

+ Open the Tizen IDE
+ Click `File > Import > General > Existing Projects into Workspace`
+ Click `Next`
+ Make sure the radio button says `Select root directory` and click `Browse`
+ Navigate to `STAVPlay` and click `Open`
+ Under projects, `STAVPlay` should be automatically selected
+ Click `Open`

Create a run profile.

+ Click `Run > Run configurations`
+ Right click `Tizen Device` and select `New`
+ Set `Widget file` to `<path to>/STAVPlay/stavplay.wgt`
+ Uncheck `Create the widget before launch`
+ Select a target device ( this will be automatically filled if the emulator is running )
+ Click `Run`

To run, right click on the project or wgt file and select `Run as > Widget on a Tizen device`.

## tv

You'll need to know the application package name and ID which can both be found in the config.xml.

Example:

`<tizen:application id="CX14UW0j9o.STAVPlay" package="CX14UW0j9o" required_version="2.3"/>`

Installation:

```
sdb connect 192.168.250.250:26101
sdb root on
sdb uninstall CX14UW0j9o
sdb install stavplay.wgt
```

Launch the widget:

`sdb shell wrt-launcher -s CX14UW0j9o.STAVPlay`

Stop the widget:

`sdb shell wrt-launcher -k CX14UW0j9o.STAVPlay`

View all logging information related to the widget:

`sdb dlog *:* | grep STAVPlay` 

## licensing
STAVPlay is licensed under the LGPL 2.1 (see [LICENSE](LICENSE.txt)). This project contains code from Samsung's [NativePlayer sample](https://github.com/SamsungDForum/NativePlayer) (see [LICENSE_SAMSUNG](LICENSE_SAMSUNG.txt)). Please check the `third/include/lib*/LICENSE` file for license information of the respective 3rd party libraries (requires downloading the libraries).
