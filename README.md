# rtp909_esp

Replicate a Roland TR909 using an ESP8266    
* Forked from http://blog.dspsynth.eu/audio-hacking-on-the-esp8266/    
* Donate : https://donorbox.org/download-source    

Changelog
* Move samples to dedicated .h file for lisibility
* Convert uint16_t to int to remove excessive warning (narrowing conversion)
* Used ICACHE_RODATA_ATTR instead of PROGMEM to save RAM.
