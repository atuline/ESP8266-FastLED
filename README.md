# ESP8266 FastLED Demos

I'm now converting some of my FastLED demos over to use MQTT messaging in order to have various parameters controlled by an Android phone running an MQTT broker and client.

## MQTT Basic Examples

If you're new to MQTT, have a look at my postings at:

* [http://tuline.com/getting-started-with-esp8266-and-mqtt/](http://tuline.com/getting-started-with-esp8266-and-mqtt/)
* [http://tuline.com/some-esp8266-and-mqtt-examples/](http://tuline.com/some-esp8266-and-mqtt-examples/)
* [http://tuline.com/iot-mqtt-panel-pro-prefixes/](http://tuline.com/some-esp8266-and-mqtt-examples/)

Youtube demo:

* [https://youtu.be/j8ajNRYaXZ8](https://youtu.be/j8ajNRYaXZ8)


The tutorials go through the steps on how to get MQTT up and running with an Android phone, provide information on these
examples, and finally discuss how to configure device prefixes.


Also, experiment with these in the following order:

* mqtt-LED-synchronous      - This is the first mqtt demonstration that looks similar to other examples online.
* mqtt-LED 					- This version supports non-blocking connectivity/re-connectivity. Wasn't easy to get this running reliably.
* mqtt-JSON					- This builds upon the previous demo by adding JSON (data exchange) support.
* mqtt-firexy				- Our first FastLED/mqtt hybrid that supports fire2012withpalette. If there's no connectivity, you still have fire.
* mqtt-sine					- Look at all the parameters you can change just for a single sine wave, including variable phase change around a variable center.

## mqtt-mesh

An almost straight migration of notamesh. Am going to re-think this for mqtt-sync Should be shortly.

## mqtt-sound

An almost straight migration of notasound. Will then re-think this for an unnamed sketch and apply much more programmable sound reaction, possibly some form of lighting ADSR.

## mqtt-sync

Sending synchronization to multiple ESP's, including delays, and then batch processing display sequences. Need to do a few proof of concept sketches first.

## Others TBD

