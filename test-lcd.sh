
#!/bin/bash

DEV_NAME=lcd_datetime

echo clear > /sys/class/lcd_ext/$DEV_NAME/cmd

while true; do
    echo 4 > /sys/class/lcd_ext/$DEV_NAME/cursor
    echo -n $(date +'%T') > /sys/class/lcd_ext/$DEV_NAME/text

    echo  42 > /sys/class/lcd_ext/$DEV_NAME/cursor
    echo -n $(date '+%d %b %Y') > /sys/class/lcd_ext/$DEV_NAME/text

    sleep 1
done
