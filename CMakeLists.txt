cmake_minimum_required(VERSION 3.12)
project(php-ext-libtrie C)

set(CMAKE_C_STANDARD 99)
# задаем переменную phproot, чтобы удобнее прописывать пути к файлам php
set(phproot /usr/local/src/php-7.2.11/)
# тут указываются каталоги, которые нужно включить в проект
# мы делаем это чтобы заставить clion понимать внутренние функции и макросы самого php
include_directories(${phproot})
include_directories(${phproot}TSRM/)
include_directories(${phproot}main/)
include_directories(${phproot}Zend/)
# без этой строки clion не сможет прочитать файл и не будет ничего индексировать

add_executable(php-ext-libtrie php_libtrie.c)