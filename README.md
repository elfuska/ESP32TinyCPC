# Tiny AMSTRAD CPC port ESP32

I've forked Ackerman's code to implement loading DSK files from SD card. Code is not the best, only a quick&dirty patch, but it just works. I've tested it only in Arduino IDE, so I removed the references to PlatformIO just in case.

I've also upgraded Espressif's esp32 libraries version to 2.0.7. This implied upgrading also fabgl library, as the included version was not compatible. I just copied and slightly modified current fabgl version (1.0.9).

Thanks to Akerman for this port. CPC was my first computer and I'm really enjoying having it running inside such a small device.

<ul>
 <li><a href='readmeEnglish.md'>English Doc</a></li>
 <li><a href='readmeSpanish.md'>Spanish Doc</a></li>
</ul>
