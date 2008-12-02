#!/bin/sh

SECS=$1

# create the key and certificate request
openssl req -new -out /tmp/cert.csr -config /etc/openssl.cnf -keyout /tmp/privkey.pem -newkey rsa:512 -passout pass:password
# remove the passphrase from the key
openssl rsa -in /tmp/privkey.pem -out /tmp/key.pem -passin pass:password
# convert the certificate request into a signed certificate
if test "$SECS" -eq "" ; then
	openssl x509 -in /tmp/cert.csr -out /tmp/cert.pem -req -signkey /tmp/key.pem -days 3650
else
	openssl x509 -in /tmp/cert.csr -out /tmp/cert.pem -req -signkey /tmp/key.pem -days 3650 -setstartsecs $SECS
fi
# Show human-readable format
openssl x509 -in /tmp/cert.pem -text -noout
# Remove unused files
rm -f /tmp/cert.csr /tmp/privkey.pem
