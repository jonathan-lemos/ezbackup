# ezbackup
My personal easy backup solution for when you don't feel like spending hours reading man pages.

## Installation

### Getting started
First, install/download all dependencies for the project:
```shell
# Most dependencies can be installed through your package manager.
# You probably have most of these installed already.
sudo pacman -S openssl ncurses libedit zlib bzip2 xz lz4

# Clone the repository locally.
git clone --recurse-submodules https://**TODO**/ezbackup.git

# The MEGA SDK will have to be built manually, since it cannot be found in most package managers.
cd ezbackup/cloud/mega_sdk
./autogen.sh
./configure
make
sudo make install
cd ../..
```

Then, build the project:
```shell
make # "make debug" builds a debug version of the project.
```

Finally, run the project:
```shell
./ezbackup
```

To use the MEGA SDK, you will need to [create an account](https://mega.nz/register) and [get your own API key](https://mega.nz/sdk).
Then, you need to create a file called cloud/keys.c that contains these values. See [cloud/keys.h](cloud/keys.h) for reference.

### Building/running the tests.
```shell
make tests
cd test
./run_all
```

### Building/viewing the documentation.
```shell
sudo pacman -S doxygen # Only necessary if you do not already have doxygen installed.
make docs
cd docs/html
firefox index.html     # firefox can be replaced with any web browser.
```

### Cleaning the project folder.
```shell
make clean
```
This reverts the project folder to its default state.

## Features
* Ncurses menu-based UI.
* Compression  (gzip, bzip2, xz, lz4)
* Encryption   (all symmetric ciphers supported by OpenSSL)
* Cloud Backup (only mega.nz supported atm)
* Incremental backups
* Include/Exclude specific directories.

## Roadmap
* Cleaning functionality.
* Progress bar refactoring (mutexes?).
* Check if local disk and cloud are synced properly (check checksum file).
* Implement compression flags properly.
* Remove redundant directories/exclude paths (e.g. "/home/user" and "/home").
* Restore functionality.
* Public/private key functionality.
* Metadata (checksum.txt encryption).
* Compression progress bars.
* Remove directories that no longer exist.
* Multithreading.

## Licensing
This project is licensed under the MIT License. See [LICENSE.txt](LICENSE.txt) for details.
Some of the project's dependencies are licensed under different licenses. See [LICENSE-THIRD-PARTY.txt](LICENSE-THIRD-PARTY.txt) for details.
