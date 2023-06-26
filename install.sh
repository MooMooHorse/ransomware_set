sudo apt-get -y update
sudo apt-get -y install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison
sudo apt-get -y install libjpeg-dev
sudo apt-get -y install python3-pip
sudo apt-get -y install f2fs-tools
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
make -j$NUM_CPUS && sudo modinstall.sh

# go to DIR/ and run make
cd $DIR/ && make