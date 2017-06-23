==============================
Installation of the standalone reader
==============================

Scope
-------------------------

You have a small setup in your lab and want to read out some data. Then you're 
in the right place. This guide shows you how to install all the prerequisites plus
the kodiaq software. This was tested on Ubuntu 16.04 but should work on any Linux
distribution provided you can provide the necessary packages.

Fetching the source code
-------------------------

Our source code is hosted on `github
<https://github.com/XENON1T/kodiaq>`_. In addition to checking out the
code, you can also use github
to report bugs and issues and keep track of the current development.
A GitHub account is needed to report issues or contribute, but the code can
be downloaded without one.

To get the latest source code::

    git clone https://github.com/XENON1T/kodiaq.git

This will create the file 'kodiaq' in whatever directory you ran the command in. 

Prerequisites for installation
-------------------------------

This software supports reading from Caen V1724 digitizers. Connection to
a Caen V1724 is mediated by either an A2818 or A3818 PCI(e) card. This card 
should be installed in your computer and configured according to the manufacturer's
instructions. See http://www.caen.it for details as well as updated driver versions.

The CAEN library CAENVMElib is also needed to interact with the digitizers. This is
available at http://www.caen.it as well. Please note that a (free) account is required
to download all CAEN software.

The following line will install all needed packages from the repositories ::

  sudo apt-get install libsnappy-dev build-essential git-core scons libprotobuf-dev protobuf-compiler automake scons git libtool libncurses5-dev unzip libssl-dev libicu-dev emacs htop screen libsasl2-dev
  
These are the packages needed both for kodiaq and for the MongoDB database that we'll install
in the next step.

In this step we'll install the MongoDB client driver. The cxx driver went through many changes
and several version during the development of this code. We require installation of the 
'legacy' (former stable) branch. This installation is easiest if we install a fixed version
of Boost instead of relying on the one from the repos. 

Install Boost as follows (warning, compilation takes a while, get a coffee). ::

  wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.zip
  unzip boost_1_55_0.zip
  cd boost_1_55_0
  sudo ./bootstrap.sh --prefix=/usr/local --with-libraries=all
  sudo ./b2 toolset=gcc cxxflags="-std=c++11" install

Next you need the MongoDB C++ driver. The following commands will compile it ::

  git clone https://github.com/mongodb/mongo-cxx-driver.git mongodb
  cd mongodb
  git checkout legacy
  sudo scons --sharedclient --c++11 --prefix=/usr --ssl  --extrapath="/usr/local/lib/" --cpppath="/usr/local/include/" --libpath="/usr/local/lib" --use-sasl-client install

On some systems you need to set a --disable-warnings-as-errors flag to suppress 
compiler warnings. Don't be scared. Embrace it.

If both of these completed correctly you can proceed to the next section. 

How to install
--------------

Assuming you meet all the requirements installation should just be ::
  
  cd kodiaq
  ./autogen.sh
  ./configure --enable-lite
  make

This will install the standalone executable into src/slave. The configure script should
automatically detect your mongodb libpbf installation. The scripts output will 
tell you which output format(s) will be supported by your kodiaq installation.

Bonus: Installing MongoDB
-------------------------

If you're lucky you already have a MongoDB database set up to use. If not you need
to install one.

The best way to install MongoDB is by Googling "how do I install MongoDB on {your OS}".
If you don't like Google or are in China, maybe Bing? But yeah, that's not so great. So 
some instructions are included here for the Google-deprived. They worked in June 2017.

Instructions for Ubuntu 16.04.

Add the official repository (which is more up to date than the Ubuntu ones) ::

  sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv EA312927

Create a list file and update ::

  echo "deb http://repo.mongodb.org/apt/ubuntu xenial/mongodb-org/3.2 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-3.2.list
  sudo apt-get update

Install ::

  sudo apt-get install -y mongodb-org

Create a configuration file at ::

  sudo nano /etc/systemd/system/mongodb.service

Fill it with the following ::

  [Unit]
  Description=High-performance, schema-free document-oriented database
  After=network.target
  
  [Service]
  User=mongodb
  ExecStart=/usr/bin/mongod --quiet --config /etc/mongod.conf
  
  [Install]
  WantedBy=multi-user.target

Start the service ::

  sudo systemctl start mongodb

Now you should be able to log in to your DB ::

  mongo

By default your database is running at localhost:27017 with no user and no security.
If you're on a private network where port 27017 is blocked to other computers then
you're done. However, if you're on a public network (or if you want your Mongo
to be remotely accessible) then you need to add security. 

Adding security is easy. First you need a user ::

  mongo
  use admin
  db.createUser({user:"admin_name", pwd:"1234",roles:["readWriteAnyDatabase","dbAdminAnyDatabase"]})
  exit

Then edit your /etc/mongod.conf file to contain the following ::

  security:
    authorization: enabled

Restart mongodb ::

  sudo systemctl restart mongodb

Now you should only be able to do anything with the database if you log in ::

  mongo -u admin_name -p 1234 localhost:27017/admin

Of course make sure you set a reasonable login and password. Please note that most
cluster administrators will still want to firewall this port so contact your local
IT staff if you have concerns.
