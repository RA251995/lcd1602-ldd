
#!/bin/bash

echo clear > /sys/class/lcd_ext/l1602-0/cmd
while true; do
    echo 4 > /sys/class/lcd_ext/l1602-0/cursor
    echo -n $(date +'%T') > /sys/class/lcd_ext/l1602-0/text
    echo  42 > /sys/class/lcd_ext/l1602-0/cursor
    echo -n $(date '+%d %b %Y') > /sys/class/lcd_ext/l1602-0/text
    sleep 1
done
