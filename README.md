# WiFi Enabled Scoreboard

This is a simple project for controlling [Large 7-Segment Displays](https://www.sparkfun.com/products/8530) that have the [Large Digit Driver](https://www.sparkfun.com/products/13279) attached to them.  


There is a simple rest server that will update the number, here's an example request:

```
curl -XPOST -H "Content-type: application/json" -d '{ "number": "42" }' 'http://ip-address-of-device/update-number'
```