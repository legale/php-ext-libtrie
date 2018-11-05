PHP_ARG_ENABLE(libtrie, whether to enable libtrie support,
[  --enable-libtrie           Enable libtrie support])

if test "$PHP_LIBTRIE" != "no"; then
  # если понадобится включить какие-то дополнительные заголовочные файлы
  # PHP_ADD_INCLUDE(libtrie/src/)
  # ключевая строка
  PHP_NEW_EXTENSION(libtrie, \
  libtrie/src/libtrie.c \
  libtrie/src/single_list.c \
  php_libtrie.c \
  , $ext_shared)
  # PHP_NEW_EXTENSION(libtrie, php_libtrie.c libtrie/src/libtrie.c libtrie/src/single_list.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
