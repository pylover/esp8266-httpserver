# FOTA 
Formware over the air transmission using easyQ.



### Build

Follow [this](https://github.com/pylover/esp8266-env) instruction 
to setup your environment.


```bash
cd esp-env
source activate.sh

cd fota 
bash gen_misc.sh
```

Or:

```bash
make clean
make assets
make flash_map6user1 

```
