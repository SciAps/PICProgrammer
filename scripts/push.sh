
#!/bin/bash

mm
adb push ../../../out/target/product/libz500/system/bin/picprogram /system/bin/
adb push ../../../out/target/product/libz500/system/bin/picflashget /system/bin/
adb push ../../../out/target/product/libz500/system/bin/pictest /system/bin/
