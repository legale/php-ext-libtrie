<?php
echo "Cоздаем дерево на 500 узлов и 500 ссылок\n\n";
$trie = yatrie_new(500, 500, 100);
echo "Готово!\n Добавляем слова, сохряняя id узлов в массив \$nodes\n";
$nodes[] = yatrie_add($trie, "ух");
$nodes[] = yatrie_add($trie, "ухо");
$nodes[] = yatrie_add($trie, "уха");
echo "Тут хорошо видно как работает дерево.\n
Первое слово из 2 букв, поэтому последний узел 2.\n
Второе слово из 3 букв, но 2 буквы совпадают с первым словом,\n
поэтому только 1 узел добавлен\n";
print_r($nodes);
print_r(yatrie_node_traverse($trie, 0));
yatrie_free($trie);
