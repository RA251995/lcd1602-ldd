
#!/bin/bash

echo clear > /sys/class/lcd_ext/l1602-1/cmd
while true; do
    echo 4 > /sys/class/lcd_ext/l1602-1/cursor
    echo -n $(date +'%T') > /sys/class/lcd_ext/l1602-1/text
    echo  42 > /sys/class/lcd_ext/l1602-1/cursor
    echo -n $(date '+%d %b %Y') > /sys/class/lcd_ext/l1602-1/text
    sleep 1
done
