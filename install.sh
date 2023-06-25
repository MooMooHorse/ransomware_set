sudo apt-get -y update
sudo apt-get -y install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison
sudo apt-get -y install libjpeg-dev
sudo apt-get -y install python3-pip
sudo apt-get -y install f2fs-tools
pip3 install -r requirements.txt
sudo apt-get -y install blktrace