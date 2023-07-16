sudo apt-get -y update
sudo apt-get -y install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison
sudo apt-get -y install libjpeg-dev
sudo apt-get -y install python3-pip
pip3 install -r requirements.txt
sudo apt-get -y install blktrace
# get current file path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# run make in DIR/impressions/
cd $DIR/impressions/ && make
# run make in DIR/linux/
cd $DIR/linux/
# copy DIR/debug/.config to DIR/linux/.config
cp $DIR/debug/.config $DIR/linux/.config
# check the number of CPUs we have
NUM_CPUS=$(grep -c ^processor /proc/cpuinfo)
# run make with the number of CPUs we have
make -j$NUM_CPUS && sudo bash modinstall.sh

# go to DIR/ and run make
cd $DIR/ && make

# I don't know why this is needed, but some certain libraries are not installed 
# if you directly install f2fs-tools 16.0
sudo apt -y install f2fs-tools
sudo apt -y remove f2fs-tools

wget https://git.kernel.org/pub/scm/linux/kernel/git/jaegeuk/f2fs-tools.git/snapshot/f2fs-tools-1.16.0.tar.gz
tar -xzf f2fs-tools-1.16.0.tar.gz
sudo apt-get -y install uuid-dev autoconf libtool libselinux1-dev
cd f2fs-tools-1.16.0
./autogen.sh
./configure
make
sudo make install
# cleanup f2fs install packages
cd ../
sudo rm -rf f2fs-tools-1.16.0
sudo rm f2fs-tools-1.16.0.tar.gz

