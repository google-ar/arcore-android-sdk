#!/usr/bin/env bash

pwd

if [[ -z "$CRYPT_PASS" ]]
then
   read -sp 'Password: ' CRYPT_PASS
   if [[ -z "$CRYPT_PASS" ]]
   then
      echo "\$CRYPT_PASS Still empty"
      exit 1
   fi
else
   echo "\$CRYPT_PASS available"
fi

pushd signing

# to encrypt
#openssl aes-256-cbc -a -salt -k "$CRYPT_PASS" -pbkdf2 -iter 100000 -in release.keystore -out release.keystore.enc
#openssl aes-256-cbc -salt -pbkdf2 -k "$CRYPT_PASS" -in all-in-one-sample/google-services.json -out all-in-one-sample/google-services.json.enc

# Ubuntu 18.04 (openssl 1.1.0g+) needs -md md5
# https://askubuntu.com/questions/1067762/unable-to-decrypt-text-files-with-openssl-on-ubuntu-18-04/1076708
echo release.keystore
echo ================
openssl aes-256-cbc -a -d -md md5 -k "$CRYPT_PASS" -in release.keystore.enc -out release.keystore
echo google-services.json
echo ====================
set +x
openssl aes-256-cbc -d -pbkdf2 -k "$CRYPT_PASS" -in ../all-in-one-sample/google-services.json.enc -out ../all-in-one-sample/google-services.json
find . -name "google-services.json"
set -x
popd 1>/dev/null