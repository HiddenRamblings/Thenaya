#!/usr/local/bin/bash

JSON="$1"
sed -n '/"amiibos":/,$p' $JSON > tmpfile && mv tmpfile $JSON
awk '!/"amiibos":/' $JSON > tmpfile && mv tmpfile $JSON
sed '/"characters":/,$d' $JSON > tmpfile && mv tmpfile $JSON
awk '!/"au"/' $JSON > tmpfile && mv tmpfile $JSON
awk '!/"eu"/' $JSON > tmpfile && mv tmpfile $JSON
awk '!/"jp"/' $JSON > tmpfile && mv tmpfile $JSON
awk '!/"na"/' $JSON > tmpfile && mv tmpfile $JSON
awk '!/"release"/' $JSON > tmpfile && mv tmpfile $JSON
sed 's/"name"\://g' $JSON > tmpfile && mv tmpfile $JSON
awk '!/{/' $JSON > tmpfile && mv tmpfile $JSON
awk '!/}/' $JSON > tmpfile && mv tmpfile $JSON
sed 's/",/"},/g' $JSON > tmpfile && mv tmpfile $JSON
sed 's/":/,/g' $JSON > tmpfile && mv tmpfile $JSON
sed 's/"0x/{0x/g' $JSON > tmpfile && mv tmpfile $JSON
sed 's/\\u00e9/e/g' $JSON > tmpfile && mv tmpfile $JSON
sed 's/\\u00c9/E/g' $JSON > tmpfile && mv tmpfile $JSON
sed 's/\\u014d/o/g' $JSON > tmpfile && mv tmpfile $JSON
