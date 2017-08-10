#!/bin/bash 

# Deployment script for kodiaq. 
# Should run without errors on ubuntu 14.04
# Will ask for your root password. Don't be scared.

# Usage: sudo bash install_script.sh
# then follow on-screen prompts

echo "Welcome to the kodiaq installation script."
echo "Install master, slave, both, or standalone?"
select ins in "master" "slave" "both" "standalone";
do 
    case $ins in
	master) 
	read -p "Monitor DB address: " -e monitor_db;
	read -p "Log DB address: " -e log_db;
	read -p "Runs DB address: " -e runs_db;
	echo "Add a new detector?"
	select detyn in "Yes" "No"
	do
	case $detyn in
            Yes ) apt-get -d install $apl; break;;
            No ) break;;
	esac
	done
	break
	;;
        slave) 
	read -p "IP/Hostname of dispatcher: " -e master_ip;
	read -p "Slave ID: " -e slave_id;
	read -p "Communication port: " -e comm_port;
	read -p "Data port: " -e data_port;
	read -p "Slave name: " -e slave_name
	break
	;;
	both) 
	    read -p "Monitor DB address: " -e monitor_db;
        read -p "Log DB address: " -e log_db;
        read -p "Runs DB address: " -e runs_db;
	echo "Add a new detector?"
        select detyn in "Yes" "No"
        do
        case $detyn in
            Yes ) echo "PUT ADD DET HERE" break;;
            No ) break;;
        esac
        done
	read -p "IP/Hostname of dispatcher: " -e master_ip;
        read -p "Slave ID: " -e slave_id;
        read -p "Communication port: " -e comm_port;
	read -p "Data port: " -e data_port;
        read -p "Slave name: " -e slave_name        
	break;;
    esac  
done
echo "Done configuring."
#exit
echo "Installing pre-requisites from package library."

sudo apt-get update
sudo apt-get upgrade
sudo apt-get install libsnappy-dev build-essential git-core scons libprotobuf-dev protobuf-compiler automake scons git libtool libncurses5-dev unzip libssl-dev libicu-dev emacs htop screen expect-dev

echo "Downloading and installing boost 1.55"

FOLDER_NAME="Boost1.55"
mkdir ${FOLDER_NAME}
cd ${FOLDER_NAME}
wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.zip
unzip boost_1_55_0.zip
cd boost_1_55_0
sudo ./bootstrap.sh --prefix=/usr/local --with-libraries=all
sudo ./b2 toolset=gcc cxxflags="-std=c++11" install
cd ../..

echo "Installing mongodb"
git clone https://github.com/mongodb/mongo-cxx-driver.git mongodb
cd mongodb
git checkout legacy
sudo scons --sharedclient --c++11 --prefix=/usr --ssl  --extrapath="/usr/local/lib/" --cpppath="/usr/local/include/" --libpath="/usr/local/lib" install

cd ..

echo "Installing kodiaq"
git clone https://github.com/XENON1T/kodiaq.git kodiaq
echo "Installing CAEN driver"
cd kodiaq/caen/CAENVMELib-2.41/lib
sudo sh install_x64
cd ../../A3818Drv-1.5.2
make
sudo make install
cd ../../..

cd kodiaq
./autogen.sh
case $ins in
    master)
	./configure --enable-master
	cd src/master
	master_config_file=MasterConfig.ini
	echo "MONITOR_DB "$monitor_db > $master_config_file
	echo "LOGS_DB "$log_db >> $master_config_file
	echo "RUNS_DB "$runs_db >> $master_config_file
	cd ../..
	;;
    slave)
	./configure --enable-slave
	cd src/slave
	slave_config_file=SlaveConfig.ini
	echo "SERVERADDR "$master_ip > $slave_config_file
	echo "COM_PORT "$comm_port >> $slave_config_file
	echo "DATA_PORT "$data_port >> $slave_config_file
	echo "NAME "$slave_name >> $slave_config_file
	echo "ID "$slave_id >> $slave_config_file
	cd ../..
	;;
    both)
	./configure --enable-all
	cd src/master
        master_config_file=MasterConfig.ini
        echo "MONITOR_DB "$monitor_db > $master_config_file
        echo "LOGS_DB "$log_db >> $master_config_file
        echo "RUNS_DB "$runs_db >> $master_config_file
        cd ../..
	cd src/slave
        slave_config_file=SlaveConfig.ini
	echo "SERVERADDR "$master_ip > $slave_config_file
        echo "COM_PORT "$comm_port >> $slave_config_file
        echo "DATA_PORT "$data_port >> $slave_config_file
        echo "NAME "$slave_name >> $slave_config_file
        echo "ID "$slave_id >> $slave_config_file
        cd ../..
	;;
    standalone) 
	./configure --enable-lite
	cd ../..
	;;
esac

echo "Compiling"
make
cd ..

sudo chown -R $USER kodiaq/*
sudo chmod

echo "Done! Enjoy your new DAQ node."
