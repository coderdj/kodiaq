==============================
The Web Interface
==============================

Introduction
--------------

Most users will only require the web interface in order to run and
monitor the DAQ. The web interface is called "emo" or "Experimental
MOnitor". The repository is here ::

  https://github.com/coderdj/emo

This is not really developed as a full fledged professional package.
You're gonna have to get your hands dirty if you want to use it. The
code is pretty specific to XENON1T but I guess you can pull parts of
it for use in other experiments if you want.

Build Notes
-----------

I'm refraining from calling this 'instructions' but more 'build notes'.
In principle these steps will get you like 80% there but you'll need to 
fill in the blanks.

First, install anaconda ::

  wget https://3230d63b5fc54e62148e-c95ac804525aac4b6dba79b00b39d1d3.ssl.cf1.rackcdn.com/Anaconda3-2.3.0-Linux-x86_64.sh
  bash Anaconda3-2.3.0-Linux-x86_64.sh
  conda create -n web python=3.4 numba matplotlib scipy
  source activate web

Do all your following work in that environment ::

  git clone https://{user}@github.com/coderdj/emo emo
  sudo apt-get install libhdf5-dev snappy libldap2-dev libsasl2-dev
  cd emo && pip install -r requirements.txt

Update your certificate with the signed one you got because you're not a loser ::

  sudo cp {your_signed_cert.crt} /usr/local/share/ca-certificates/cert.crt
  sudo update-ca-certificates

Run the test server ::

  python manage.py migrate
  python runserver

Now you should be able to log in with a browser. But **for the love of all
things holy** do not use this test server in production. It is very insecure.

You probably want to deploy with apache and mod_wsgi. Trust me, you're not the 
first, or even one of the first 10,000, who wants to deploy a Django website
with apache. Google is your friend.
