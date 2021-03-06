// php-ext-libtrie v0.1.3
#include "php_libtrie.h"



//тут хранится id ресурса, присваиваемый при его регистрации PHP
static int le_libtrie;

/**
 * @brief обходит все ветви дерева, начиная с заданного узла, и выводит в массив все
 *         слова, встреченные на своем пути
 * @param trie                  : resource
 * @param node_id               : int
 * @param head (необязательный) : string строка, которая будет
 *                                добавлена в начало каждого найденного слова
 * @return array
 */
PHP_FUNCTION (yatrie_node_traverse) {
    trie_s *trie; //указатель для trie
    words_s *words; //структура для сохранения слов из trie
    string_s *head_libtrie; //структура для передачи префикса

    zval * resource; //указатель для zval ресурса
    zend_long node_id; //начальный узел
    zend_string *head = NULL; //указатель на строку префикс


    //получаем параметры стандартным для php методом - через макросы
    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_RESOURCE(resource)
        Z_PARAM_LONG(node_id)
    Z_PARAM_OPTIONAL
        Z_PARAM_STR(head)
    ZEND_PARSE_PARAMETERS_END();


    //получим наше дерево из ресурса
    trie = (trie_s *) zend_fetch_resource(Z_RES_P(resource), PHP_LIBTRIE_RES_NAME, le_libtrie);

    //для сохранения слов из trie функция yatrie_node_traverse() использует специальную структура words_s
    //выделим память под нее
    words = (words_s *) calloc(1, sizeof(words_s));
    words->counter = 0; //установим счетчик слов на 0

    //нужна еще 1 структура для передачи префикса
    head_libtrie = (string_s *)calloc(1, sizeof(string_s));

    //если head задан
    if(head != NULL) {
        head_libtrie->length = ZSTR_LEN(head); //присвоим длину
        memcpy(&head_libtrie->letters, ZSTR_VAL(head), ZSTR_LEN(head)); //скопируем строку в head_libtrie
    }
    //теперь получим слова из trie
    yatrie_node_traverse(words, (uint32_t) node_id, head_libtrie, trie);
    //теперь создадим PHP массив, размер возьмем из счетчика слов в words
    array_init_size(return_value, words->counter);

    //добавим слова в массив php
    while (words->counter--) {
        //поскольку в trie буквы хранятся в виде кодов, нужно декодировать их
        //это массив для декодированного слова
        uint8_t dst[256];
        //функция из библиотеки libtrie
        decode_string(dst, words->words[words->counter]);

        //эта функция Zend API, которая добавляет в массив элемент с типом php string из типа Си char *
        add_next_index_string(return_value, (const char *) dst);
    }
    //теперь надо освободить память выделенную под words и head_libtrie
    free(words);
    free(head_libtrie);
}


/**
 * @brief Возвращает все дочерние узлы определенного узла. Элемент 0 - флаг окончания слова.
 * @param trie                  : resource
 * @param node_id               : int
 * @param letters_flag          : bool Позволяет получить ассоц. массив с ключами в виде букв, а не номеров битов
 * @return array
 */
PHP_FUNCTION (yatrie_node_get_children) {
    trie_s *trie; //указатель для trie
    children_s *children; //структура для сохранения детей узла
    zval * resource; //указатель для zval ресурса
    zend_long node_id; //id узла
    zend_bool letters_flag = 0; //если флаг установлен, будут возвращен ассоц. массив с буквами в виде ключей

    //получаем аргументы из PHP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl|b", &resource, &node_id, &letters_flag) == FAILURE) {
        RETURN_NULL(); //возвращаем null в случае неудачи
    }

    //получим наше дерево из ресурса
    trie = (trie_s *) zend_fetch_resource(Z_RES_P(resource), PHP_LIBTRIE_RES_NAME, le_libtrie);

    //выделение памяти под структуру детей узла
    children = (children_s *) calloc(1, sizeof(children_s));


    //теперь получим слова из trie
    yatrie_node_get_children(children, (uint32_t) node_id, trie);

    //теперь создадим PHP массив, размер возьмем из самой структуры для хранения детей узла
    // +1 потому что флаг окончания слова не входит в length


    //добавим данные из блока letters, значения в этом блоке содержат номера поднятых в маске узла битов
    //если есть флаг окончания слова, то 0 элемент будет содержать 1 (номер бита флага листа)
    if(children->letters[0] == 1) {
        array_init_size(return_value, children->length + 1);
        add_index_long(return_value, 0, 1);
        for (uint32_t i = 1; i <= children->length; ++i) {
            //добавляем id узлов букв, в те индексы, которые соответствуют этим буквам
            add_index_long(return_value, children->letters[i], children->nodes[i - 1]);
        }
        //в противном случае
    }else{
        array_init_size(return_value, children->length);
        for (uint32_t i = 0; i < children->length; ++i) {
            //добавляем id узлов букв, в те индексы, которые соответствуют этим буквам
            add_index_long(return_value, children->letters[i], children->nodes[i]);
        }
    }

    //теперь надо освободить память выделенную под words и head_libtrie
    free(children);
}



/**
 * @brief Добавляет в trie слово и возвращает node_id последней буквы добавленного слова
 * @param trie  : resource
 * @param word  : string
 * @return node_id : int
 */
PHP_FUNCTION (yatrie_add) {
    trie_s *trie; //указатель для дерева
    zval *resource; //указатель для zval структуры с ресурсом
    unsigned char *word = NULL; //указатель для строки добавляемого слова
    size_t word_len; //длина слова word

    uint32_t node_id; //id последнего узла, добавленного слова

    //получаем аргументы
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &resource, &word, &word_len) == FAILURE) {
        RETURN_NULL();
    }



    /*получаем ресурс из недр PHP, функция принимает:
     * 1 аргументом сам ресурс PHP (не zval, а именно ресурс),
     * 2 аргументом имя ресурса
     * я установил его через константу в заголовочном файле
     * 3 аргументом числовой id ресурса, который был присвоен при регистрации ресурса
     * Функция возвращает указатель типа void *, поэтому надо его привести к правильному типу trie_s
     *
     * В PHP5 в этом месте использовался макрос ZEND_FETCH_RESOURCE(), который почему-то решили убрать в PHP7.
     */
    trie = (trie_s *) zend_fetch_resource(Z_RES_P(resource), PHP_LIBTRIE_RES_NAME, le_libtrie);


    /* добавим слово в trie
     * первый аргумент - указатель на строку добавляемого слова
     * второй аргумент - id узла с которого начать добавление, мы добавляем с корневого узла
     * третий аргумент - указатель на наше дерево.
     */
    node_id = yatrie_add(word, 0, trie);

    //возвращаем числовое значение
    RETURN_LONG(node_id);
}

/**
 * @brief Возвращает id узла последней буквы заданного слова
 * @param trie  : resource
 * @param word  : string
 * @param parent_id  : int родительский узел, с которого начнется проход по дереву
 * @return node_id : int
 */
PHP_FUNCTION (yatrie_get_id) {
    trie_s *trie; //указатель для дерева
    zval *resource; //указатель для zval структуры с ресурсом
    unsigned char *word = NULL; //указатель для строки искомого слова
    size_t word_len; //длина слова word
    zend_long parent_id = 0; //id родительского узла

    uint32_t node_id; //id последнего узла, добавленного слова

    //получаем аргументы
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &resource, &word, &word_len, &parent_id) == FAILURE) {
        RETURN_NULL();
    }
    //получаем указатель на дерево
    trie = (trie_s *) zend_fetch_resource(Z_RES_P(resource), PHP_LIBTRIE_RES_NAME, le_libtrie);

    node_id = yatrie_get_id(word, parent_id, trie);

    //возвращаем id узла, 0 означает, что ничего не найдено
    RETURN_LONG(node_id);
}

/**
 * @brief Возвращает true если последняя буква заданного слова является листом
 * @param trie  : resource
 * @param word  : string
 * @param parent_id  : int родительский узел, с которого начнется проход по дереву
 * @return node_id : int
 */
PHP_FUNCTION (yatrie_is_leaf) {
    trie_s *trie; //указатель для дерева
    zval *resource; //указатель для zval структуры с ресурсом
    unsigned char *word = NULL; //указатель для строки искомого слова
    size_t word_len; //длина слова word
    zend_long parent_id = 0; //id родительского узла

    zend_long node_id; //id последнего узла, добавленного слова

    //получаем аргументы
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &resource, &word, &word_len, &parent_id) == FAILURE) {
        RETURN_NULL();
    }
    //получаем указатель на дерево
    trie = (trie_s *) zend_fetch_resource(Z_RES_P(resource), PHP_LIBTRIE_RES_NAME, le_libtrie);

    if(parent_id == 0){
        node_id = BIT_GET(*NODE_GET(0, trie).mask[0],1) ? 1 : -1;
    }else{
        node_id = yatrie_get_id(word, parent_id, trie);
        if(node_id){
            node_id = BIT_GET(*NODE_GET(node_id, trie).mask[0],1) ? 1 : -1;
        }
    }

    //возвращаем id узла, 0 означает, что ничего не найдено
    RETURN_LONG(node_id);
}


/**
 * @brief Возвращает массив всех найденных узлов заданного слова, а также узлы-листья
 * @param trie  : resource
 * @param word  : string
 * @param parent_id (необязательный) : int родительский узел, с которого начнется проход по дереву
 * @return word_nodes : array отрицательные значения для узлов без листа, положительные для узлов с листами
 */
PHP_FUNCTION (yatrie_get_word_nodes) {
    trie_s *trie; //указатель для дерева
    zval *resource; //указатель для zval структуры с ресурсом
    unsigned char *word = NULL; //указатель для строки искомого слова
    size_t word_len; //длина слова word
    zend_long parent_id = 0; //id родительского узла
    word_nodes_s word_nodes = {};

    //получаем аргументы
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &resource, &word, &word_len, &parent_id) == FAILURE) {
        RETURN_NULL();
    }
    //получаем указатель на дерево
    trie = (trie_s *) zend_fetch_resource(Z_RES_P(resource), PHP_LIBTRIE_RES_NAME, le_libtrie);

    yatrie_get_word_nodes(&word_nodes, word, 0, trie);
    array_init_size(return_value, word_nodes.length);
    for(uint32_t i = 0;word_nodes.nodes[i]; ++i){
        //printf("%i %i %i\n", word_nodes.letters[i], i, word_nodes.nodes[i]);
        if(word_nodes.letters[i] == 1){
            add_next_index_long(return_value, (zend_long)word_nodes.nodes[i]);
        } else {
            add_next_index_long(return_value, -((zend_long)word_nodes.nodes[i]));
        }
    }
    return;
}




/**
 * @brief Удаляет дерево из памяти
 * @param trie  : resource
 * @return true/false : bool
 */
PHP_FUNCTION (yatrie_free) {
    zval *resource; //указатель для zval структуры с ресурсом

    //получаем аргумент типа ресурс
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &resource) == FAILURE) {
        RETURN_FALSE;
    }

    /* тут вызывается закрытие ресурса, на входе принимается zend_resource,
     * который сначала надо достать из zval. Это и делает макрос Z_RES_P()
     */
    if (zend_list_close(Z_RES_P(resource)) == SUCCESS) {
        //макрос пишет true в return_vale и делает return
        RETURN_TRUE;
    }
    //макрос пишет false в return_vale и делает return
    RETURN_FALSE;
}

/**
 * @brief деструктор ресурса, он принимает на входе указатель на ресурс и закрывает его
 * @param rsrc  : zend_resource * указатель
 * @return void
 */
static void php_libtrie_dtor(zend_resource *rsrc TSRMLS_DC) {
    //тут мы берем указатель на trie из ресурса
    trie_s *trie = (trie_s *) rsrc->ptr;
    //тут выполняется функция библиотеки, которая освобождает память,
    // выделенную для trie
    yatrie_free(trie);
}

/**
 * @brief Создает trie и возвращает его ресурс
 * @param max_nodes             : int
 * @param max_refs              : int
 * @param max_deallocated_size  : int
 * @return trie                 : resource
 */
PHP_FUNCTION (yatrie_new) {
    /* Это указатель на дерево */
    trie_s *trie;
    //это переменные для аргументов функции
    zend_long
    max_nodes; //максимальное кол-во узлов доступное в нашем дереве
    zend_long
    max_refs; /* максимальное кол-во ссылок в дереве.
 * Потребность зависит от плотности записи слов в дерево. Кол-во узлов +25% должно хватать на любое дерево.
 * Например, словарь русского языка OpenCorpora ~3млн. слов укладывается в 5млн. узлов и 5млн. ссылок */
    zend_long
    max_deallocated_size; /* максимальный размер освобождаемых участков в блоке ссылок
 * Зависит от плотности записи. Всего у нас 96 бит в маске узла, 1 бит зарезервирован, остается 95.
 * Значит для любого узла участок памяти в блоке ссылок не может быть больше 95, что значит,
 * что макс. размер освобожденного участка ссылок не может быть больше 94. */

    //получаем аргументы из PHP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lll", &max_nodes, &max_refs, &max_deallocated_size) == FAILURE) {
        RETURN_NULL();
    }

    //создаем дерево и записываем его адрес в памяти в созданный для этого указатель
    trie = yatrie_new((uint32_t) max_nodes, (uint32_t) max_refs, (uint32_t) max_deallocated_size);

    //Если не удалось - завершаем работу
    if (!trie) {
        RETURN_NULL();
    }
    //тут выполняется 2 действия
    /* функция zend_register_resource() регистрирует ресурс в недрах Zend,
     * пишет номер этого ресурса в глобальную переменную le_libtrie, а макрос ZVAL_RES()
     * сохраняет созданный ресурс в zval return_value */
    ZVAL_RES(return_value, zend_register_resource(trie, le_libtrie));
}

/**
 * @brief Загружает trie из файла и возвращает его ресурс
 * @param filepath  : string
 * @return trie     : resource
 */
PHP_FUNCTION (yatrie_load) {
    /* Это указатель на дерево */
    trie_s *trie;
    //это переменные для аргументов функции
    unsigned char *filepath = NULL; //указатель для пути к файлу словаря
    size_t filepath_len; //длина строки

    //получаем аргументы из PHP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &filepath, &filepath_len) == FAILURE) {
        RETURN_NULL();
    }

    //создаем дерево и записываем его адрес в памяти в созданный для этого указатель
    trie = yatrie_load(filepath);

    //Если не удалось - завершаем работу
    if (!trie) {
        php_error_docref(NULL, E_WARNING, "failed to open trie file");
        RETURN_NULL();
    }
    //тут выполняется 2 действия
    /* функция zend_register_resource() регистрирует ресурс в недрах Zend,
     * пишет номер этого ресурса в глобальную переменную le_libtrie, а макрос ZVAL_RES()
     * сохраняет созданный ресурс в zval return_value */
    ZVAL_RES(return_value, zend_register_resource(trie, le_libtrie));
}

/**
 * @brief Сохраняет trie в файл
 * @param filepath  : string
 * @return : int trie_file_size
 */
PHP_FUNCTION (yatrie_save) {
    /* Это указатель на дерево */
    trie_s *trie;
    zval *resource;
    //это переменные для аргументов функции
    unsigned char *filepath = NULL; //указатель для пути к файлу словаря
    size_t filepath_len; //длина строки
    size_t trie_file_size; //размер словаря

    //получаем аргументы из PHP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rs",  &resource, &filepath, &filepath_len) == FAILURE) {
        RETURN_NULL();
    }
    //получаем указатель на trie из указателя на zval
    trie = (trie_s *) zend_fetch_resource(Z_RES_P(resource), PHP_LIBTRIE_RES_NAME, le_libtrie);

    //создаем дерево и записываем его адрес в памяти в созданный для этого указатель
    trie_file_size = yatrie_save(filepath, trie);

    //Если не удалось - завершаем работу
    if (!trie_file_size) {
        RETURN_NULL();
    }else{
        RETURN_LONG(trie_file_size);
    }
}

PHP_FUNCTION(yatrie_strrev) {
    zend_string *str; //указатель на передаваемую в функцию структуру строки
    const unsigned char *s;
    unsigned char *e;
    unsigned char *p;
    zend_string *n; //указатель на результирующую структуру строки
    //получаем переданную строку в указатель str
    ZEND_PARSE_PARAMETERS_START(1, 1)
            Z_PARAM_STR(str)
    ZEND_PARSE_PARAMETERS_END();
    n = zend_string_alloc(ZSTR_LEN(str), 0); //выделяем память под новую строку
    p = ZSTR_VAL(n); //передаем адрес строки из структуры во временный указатель новой строки
    e = p + ZSTR_LEN(str); //перематываем новый указатель на конец строки
    s = ZSTR_VAL(str); //передаем адрес строки из структуры во временный указатель старой строки
    *e-- = '\0'; // пишем терминатор конца строки в конец строки

    while (e >= p) { //крутим цикл пока перемотанный указатель не дойдет до первого блока новой строки
        if (*s < 128) { //1 byte char       max 0b01111111
            *e-- = *s++;
        }else if(*s < 224) { //2 byte char  max 0b11011111
            *e-- = *(s+1);
            *e-- = *s;
            s += 2;
        } else if (*s < 240) { //3 byte     max 0b11101111
            *e-- = *(s+2);
            *e-- = *(s+1);
            *e-- = *s;
            s += 3;
        } else { //4 byte                   max ob11110111
            *e-- = *(s+3);
            *e-- = *(s+2);
            *e-- = *(s+1);
            *e-- = *s;
            s += 4;
        }
    }
    //возвращаем новую строку
    RETVAL_NEW_STR(n);
}


/* Convert a multibyte string to an array. If split_length is specified,
 * break the string down into chunks each split_length characters long. */
PHP_FUNCTION(yatrie_str_split){
    zend_string *str;
    zend_long split_length = 1;
    const unsigned char *p, *last; //string pointers

    ZEND_PARSE_PARAMETERS_START(1, 2)
            Z_PARAM_STR(str)
            Z_PARAM_OPTIONAL
            Z_PARAM_LONG(split_length)
    ZEND_PARSE_PARAMETERS_END();

    if (split_length <= 0) {
        php_error_docref(NULL, E_WARNING, "The length of each segment must be greater than zero");
        RETURN_FALSE;
    }

    if (0 == ZSTR_LEN(str) || (size_t)split_length >= ZSTR_LEN(str)) {
        array_init_size(return_value, 1);
        add_next_index_stringl(return_value, ZSTR_VAL(str), ZSTR_LEN(str));
        return;
    }

    //minimum array size is string length / 4 bytes / split_length
    array_init_size(return_value, (ZSTR_LEN(str) >> 2) / (size_t)split_length);


	//MACROS to create precount array
	#define B2(n) n,n
	#define B4(n) B2(n),B2(n)
	#define B8(n) B4(n),B4(n)
	#define B16(n) B8(n),B8(n)
	#define B32(n) B16(n),B16(n)
	#define B64(n) B32(n),B32(n)
	#define B128(n) B64(n),B64(n)

	/*
	 * 1 byte  max 0b01111111 (0-127 128) ASCII
	 * 1 byte  max 0b10111111 (128-191 64) data
	 * 2 bytes max 0b11011111 (191-223 32) control char
	 * 3 bytes max 0b11101111 (224-239 16) control char
	 * 4 bytes max ob11110111 (240-247 8) control char
	 * 5 bytes max ob11111011 (248-251 4) control char
	 * 6 bytes max ob11111101 (252-253 2) control char
	 * 1 byte  max ob11111111 (254-255 2) out of bounds  */
	const unsigned char byte[256] = {B128(1),B64(1),B32(2),B16(3),B8(4),B4(5),B2(6),B2(1)};

    p = ZSTR_VAL(str);
    last = p + ZSTR_LEN(str);

    while (p < last) {
    	const unsigned char *chunk_p = p;
        uint32_t chunk_length = 0;
        for(uint32_t char_count = 0; char_count < split_length; ++char_count) {
			chunk_length += byte[*p];
            p += byte[*p];
		}
        add_next_index_stringl(return_value, chunk_p, chunk_length);
    }
    return;
}



PHP_FUNCTION (confirm_libtrie_compiled) {
    const char *c = NULL;
    size_t len;
    zend_string *str;

    ZEND_PARSE_PARAMETERS_START(0, 1)
            Z_PARAM_OPTIONAL
            Z_PARAM_STR(str)
    ZEND_PARSE_PARAMETERS_END();

    len = ZSTR_LEN(str);
    c = ZSTR_VAL(str);


    str = strpprintf(0, "Congratulations! arg passed: %s len: %d", c, len);

    RETURN_STR(str);
}

PHP_FUNCTION (my_array_fill) {
    //сначала объявляем все переменные, которые нам тут понадобятся
    //в любую функцию передаются 2 аргумента:
    //указатели zend_execute_data *execute_data, zval *return_value
    //через первый указатель функция получает аргументы, а через второй отдает данные

    //zend_long это int64_t на x64 системах и int32_t на x86 системах

    //число передается в функцию с типом zend_long
    zend_long
    start_index; //1 аргумент число,
    zend_long
    num; //2 тоже число
    zval * value; //поскольку у нас mixed тип, то берем zval, которые может хранить любой тип

    //получаем аргументы в объявленные переменные, тут сразу проходит проверка на количество и тип аргументов
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz",
                              &start_index, &num, &value) == FAILURE) {
        /*наши функции ничего не выводят
         * поэтому все макросы RETURN_ просто пишут в
         * return_value результат и прерывают функцию */
        RETURN_FALSE;
    }

    //проводим проверку второго аргумента, где задается кол-во выводимых элементов массива
    if (num <= 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "argument 2 must be > 0"); //очередной макрос для вывода ошибки
        RETURN_FALSE;
    }

    //zval *return_value уже есть, поэтому сразу в нем и инициализируем массив
    //этот макрос принимает на входе указатель на zval, в котором надо сделать массив, и кол-во элементов
    //приводим тип из zend_long в unsigned int32.
    // Размер ключей массива первое значение + кол-во. Т.е. если первое 1, а надо всего 3, то массив будет из 4 элементов
    array_init_size(return_value, (uint32_t) (start_index + num));

    //добавляем через цикл, начиная с начального, заканчивая последним
    for (zend_long i = start_index, last = start_index + num; i < last; ++i) {
        //копируем указатель нашего zval со входа в каждый элемент массива
        add_index_zval(return_value, i, value);
    }
    //функция ничего не возвращает, а массив уже записан в return_value
    return;
}

//Регистрируем в PHP функцию деструктор нашего ресурса trie
PHP_MINIT_FUNCTION (libtrie) {
    le_libtrie = zend_register_list_destructors_ex(
            php_libtrie_dtor,
            NULL, PHP_LIBTRIE_RES_NAME, module_number);
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION (libtrie) {
    return SUCCESS;
}

PHP_RINIT_FUNCTION (libtrie) {
#if defined(COMPILE_DL_LIBTRIE) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION (libtrie) {
    return SUCCESS;
}

PHP_MINFO_FUNCTION (libtrie) {
    php_info_print_table_start();
    php_info_print_table_header(2, "libtrie support", "enabled");
    php_info_print_table_end();
}

const zend_function_entry libtrie_functions[] = {
        PHP_FE(confirm_libtrie_compiled, NULL)        /* For testing, remove later. */
        PHP_FE(my_array_fill, NULL)
        PHP_FE(yatrie_new, NULL)
        PHP_FE(yatrie_load, NULL)
        PHP_FE(yatrie_save, NULL)
        PHP_FE(yatrie_free, NULL)
        PHP_FE(yatrie_add, NULL)
        PHP_FE(yatrie_get_id, NULL)
        PHP_FE(yatrie_node_traverse, NULL)
        PHP_FE(yatrie_node_get_children, NULL)
        PHP_FE(yatrie_get_word_nodes, NULL)
        PHP_FE(yatrie_strrev, NULL)
        PHP_FE(yatrie_str_split, NULL)
        PHP_FE(yatrie_is_leaf, NULL)
        PHP_FE_END    /* Must be the last line in libtrie_functions[] */
};

zend_module_entry libtrie_module_entry = {
        STANDARD_MODULE_HEADER,
        "libtrie",
        libtrie_functions,
        PHP_MINIT(libtrie),
        PHP_MSHUTDOWN(libtrie),
        PHP_RINIT(libtrie),        /* Replace with NULL if there's nothing to do at request start */
        PHP_RSHUTDOWN(libtrie),    /* Replace with NULL if there's nothing to do at request end */
        PHP_MINFO(libtrie),
        PHP_LIBTRIE_VERSION,
        STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_LIBTRIE
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(libtrie)
#endif
