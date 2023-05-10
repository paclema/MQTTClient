Store here your certs for mqtt broker

Load certificate file:
But you must convert it to .der
openssl x509 -in ./client.crt -out ./cert.der -outform DER

Load private key:
But you must convert it to .der
openssl rsa -in ./client.key -out ./private.der -outform DER
