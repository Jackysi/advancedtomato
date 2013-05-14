#!/bin/sh
SECS=1262278080

cd /etc

NVCN=`nvram get https_crt_cn`
if [ "$NVCN" == "" ]; then
	NVCN=`nvram get router_name`
fi

cp -L openssl.cnf openssl.config

I=0
for CN in $NVCN; do
        echo "$I.commonName=CN" >> openssl.config
        echo "$I.commonName_value=$CN" >> openssl.config
        I=$(($I + 1))
done

# KDB 2013/05/12 http://support.microsoft.com/kb/2661254 - Windows now requires RSA keys 1024+ length
# Previous fixes have included increasing Tomato key to 2048 bits, but this takes 1512 bytes more NVRAM
# if key saved between router reboots.

# create the key and certificate request
openssl req -new -out /tmp/cert.csr -config openssl.config -keyout /tmp/privkey.pem -newkey rsa:1024 -passout pass:password
# remove the passphrase from the key
openssl rsa -in /tmp/privkey.pem -out key.pem -passin pass:password
# convert the certificate request into a signed certificate
openssl x509 -in /tmp/cert.csr -out cert.pem -req -signkey key.pem -setstartsecs $SECS -days 3653 -set_serial $1

#	openssl x509 -in /etc/cert.pem -text -noout

rm -f /tmp/cert.csr /tmp/privkey.pem openssl.config
