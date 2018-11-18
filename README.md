# php-ext-libtrie v0.1.0
php extension libtrie C library implementation

DOWNLOAD
```
git clone https://github.com/legale/php-ext-libtrie
cd php-ext-libtrie 
git submodule init && git submodule update
```

COMPILATION
```
phpize && ./configure
make
```
DEMO
```
php -d extension=modules/libtrie.so libtrie_test.php
```
