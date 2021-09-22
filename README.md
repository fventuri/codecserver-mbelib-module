# mbelib module for OpenWebRX Codec Server

A module for OpenWebRX Codec Server that adds support for mbelib to allow Digital Voice decoding in software

- more info about OpenWebRX Codec Server can be found [here](https://github.com/jketterl/codecserver)
- more info about mbelib can be found [here](https://github.com/szechyjs/mbelib)

This module currently supports the following Digital Voice protocols:
  - D-STAR
  - DMR
  - YSF (both DN and VW mode)

## How to build

Prerequisites for building this module are a successful build and install of both OpenWebRX Codec Server and mbelib. Please follow the instructions in the links provided above to build and install both dependencies.

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


## Credits

- Many thanks to Jakob Ketterl, DD5JFK for all his hard work on OpenWebRX and Codec Server, and especially for digiham version 0.4 (https://github.com/jketterl/digiham/tree/release-0.4); all the parameters for deinterleaving and descrambling the various Digital Voice protocols come from that version of digiham.
- Also many thanks to the anonymous author(s) of mbelib for all their work researching, implementing, and testing the codecs used in these Digital Voice protocols.


## Copyright

(C) 2021 Franco Venturi - Licensed under the GNU GPL V3 (see &lt;LICENSE&gt;)
