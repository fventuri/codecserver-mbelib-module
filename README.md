# mbelib module for OpenWebRX Codec Server

A module for OpenWebRX Codec Server that adds support for mbelib to allow Digital Voice decoding in software

- more info about OpenWebRX Codec Server can be found [here](https://github.com/jketterl/codecserver)
- more info about mbelib can be found [here](https://github.com/szechyjs/mbelib)

This module currently supports the following Digital Voice protocols:
  - D-STAR
  - DMR
  - YSF (both DN and VW mode)

## How to build

NOTE: for detailed instructios on on how to build and install this Codec Server module on the OpenWebRX Raspberry Pi images, please see the section 'How to build - OpenWebRX Raspberry Pi images' below.

Prerequisites for building this module are a successful build and install of both OpenWebRX Codec Server and mbelib. Please follow the instructions in the links provided above to build and install both dependencies.

_IMPORTANT_: OpenWebRX Code Server requires the system account 'codecserver' in order to run; that account is very likely not on your system the first time you run codecserver. To create that system account you can use a command like this:
```
sudo useradd -r -d /nonexistent -c 'OpenWebRX Codec Server' codecserver
```

Once all the prerequisites are built, installed, and running as expected, these are the steps to build and install 'codecserver-build-module':

```
git clone https://github.com/fventuri/codecserver-mbelib-module.git
cd codecserver-mbelib-module
mkdir build
cd build
cmake ..
make
sudo make install
cd ..
```

You will also need the Codec Server configuration file (typically located at '/usr/local/etc/codecserver/codecserver.conf'): if you do not have that directory and file already, you can copy it with the commands:
```
sudo mkdir -p /usr/local/etc/codecserver
sudo cp conf/codecserver.conf /usr/local/etc/codecserver
```
If you already have a Codec Server configuration file, just append the '[device:mbelib]' section to the existing configuration file.

Finally restart OpenWebRX Codec Server service:
```
sudo systemctl restart codecserver
```


## How to build - OpenWebRX Raspberry Pi images

Since the OpenWebRX Raspberry Pi images already contain Codec Server, some of the steps above are not needed (and other steps are slightly different).

For those who want to add this module to an existing OpenWebRX Raspberry Pi image, these are the exact steps and commands I used on mine:

```
(assign IP address)
(enable SSH)
(remote login as user pi)

sudo apt update
sudo apt full-upgrade
sudo reboot

(after reboot)

git clone https://github.com/szechyjs/mbelib.git
cd mbelib
mkdir build
cd build
cmake ..
make
sudo make install
cd
sudo ldconfig

sudo apt install libcodecserver-dev libprotobuf-dev protobuf-compiler

git clone https://github.com/fventuri/codecserver-mbelib-module.git
cd codecserver-mbelib-module
mkdir build
cd build
cmake -DCMAKE_INSTALL_LIBDIR='/usr/lib/arm-linux-gnueabihf' ..
make
sudo make install
cd
sudo ldconfig

{ echo; echo '[device:mbelib]'; echo 'driver=mbelib'; echo 'unvoiced_quality=3'; } | sudo tee -a /etc/codecserver/codecserver.conf
sudo systemctl restart codecserver
```


## Related work

Another piece of software very similar to this one developed by knatterfunker is [codecserver-softmbe](https://github.com/knatterfunker/codecserver-softmbe).


## Credits

- Many thanks to Jakob Ketterl, DD5JFK for all his hard work on OpenWebRX and Codec Server, and especially for digiham version 0.4 (https://github.com/jketterl/digiham/tree/release-0.4); all the parameters for deinterleaving and descrambling the various Digital Voice protocols come from that version of digiham.
- Also many thanks to the anonymous author(s) of mbelib for all their work researching, implementing, and testing the codecs used in these Digital Voice protocols.


## Copyright

(C) 2021 Franco Venturi - Licensed under the GNU GPL V3 (see [LICENSE](LICENSE))
