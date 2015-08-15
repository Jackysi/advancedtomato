# Prequesites to build AdvancedTomato on Debian Wheezy 7.7 X64

- 1.) Enable i386 support, write via terminal: <pre>sudo dpkg --add-architecture i386</pre>

- 2.) Download & install following packages:
<pre>sudo apt-get install build-essential autoconf libncurses5 libncurses5-dev m4 bison flex libstdc++6-4.4-dev g++-4.4 g++ libtool sqlite gcc g++ binutils patch bzip2 flex bison make gettext unzip zlib1g-dev libc6 gperf sudo automake automake1.9 git-core lib32stdc++6 libncurses5 libncurses5-dev m4 bison gawk flex libstdc++6-4.4-dev g++-4.4-multilib g++ git gitk zlib1g-dev autopoint libtool shtool autogen mtd-utils gcc-multilib gconf-editor lib32z1-dev pkg-config gperf libssl-dev libxml2-dev libelf1:i386 make intltool libglib2.0-dev libstdc++5 texinfo dos2unix xsltproc libnfnetlink0 libcurl4-openssl-dev libxml2-dev libgtk2.0-dev libnotify-dev libevent-dev mc</pre>

- 3.) For transmission you will need automake 1.13.2 which u can get from here:
<pre>wget http://tomato.groov.pl/download/K26RT-N/testing/automake_1.13.2-1ubuntu1_all.deb
dpkg -i automake_1.13.2-1ubuntu1_all.deb</pre>

- 4.) Clone repository to your hard drive
<pre>git clone git@github.com:Jackysi/advancedtomato2.git</pre>

- 5.) Now you need to link toolchains to the git repo you downloaded
<pre>sudo ln -s $HOME/advancedtomato2/tools/brcm /opt/brcm
sudo ln -s /opt/brcm/K26/hndtools-mipsel-uclibc-4.2.4 /opt/brcm/hndtools-mipsel-linux
sudo ln -s /opt/brcm/K26/hndtools-mipsel-uclibc-4.2.4 /opt/brcm/hndtools-mipsel-uclibc</pre>

- 6.) Add toolchains to your $PATH
<pre>echo "export PATH=$PATH:/opt/brcm/hndtools-mipsel-linux/bin:/opt/brcm/hndtools-mipsel-uclibc/bin:/sbin/" >> ~/.profile && source ~/.profile</pre>

- 7.) READY! Too see the options for builds (routers and packages) do:
<pre>cd advancedtomato2/release/src-rt && make help     # For Tomato RT-N builds
cd advancedtomato2/release/src-rt-6.x/ && make help		# For Tomato RT-AC builds</pre>

- 8.) To compile specific firmware (E.g. RT-AC66U) run this:
<pre>cd advancedtomato/release/src-rt-6.x/make ac66z V1=AT-RT-AC6x V2=2.4-124</pre>
After the compile process is done, you will find your router image inside "$HOME/advancedtomato2/release/src-rt-6.x/image"

Thats it!
