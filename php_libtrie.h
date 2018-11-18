#ifndef PHP_LIBTRIE_H
#define PHP_LIBTRIE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h> //тут для макроса va_start()
#include <inttypes.h> //тут стандартные числовые типы
#include <string.h> //mymcpy()


//нужные константы
#if defined(__GNUC__) && __GNUC__ >= 4
# define ZEND_API __attribute__ ((visibility("default")))
# define ZEND_DLEXPORT __attribute__ ((visibility("default")))
#else
# define ZEND_API
# define ZEND_DLEXPORT
#endif
# define SIZEOF_SIZE_T 8 //нужна для макроса ZVAL_COPY_VALUE()

#ifndef ZEND_DEBUG
#define ZEND_DEBUG 0
#endif

//тут декларации того, что используется в нашем расширении
#include "libtrie/src/libtrie.h"
#include "php.h"
#include "php_ini.h"
#include "zend.h"
#include "zend_types.h" //ZVAL_COPY_VALUE
#include "ext/standard/info.h"

#include "zend_API.h"
#include "zend_modules.h"
#include "zend_string.h"
#include "spprintf.h"


extern zend_module_entry libtrie_module_entry;
#define phpext_libtrie_ptr &libtrie_module_entry

#define PHP_LIBTRIE_VERSION "0.1.0" /* Replace with version number for your extension */
#define PHP_LIBTRIE_RES_NAME "libtrie data structure" /* PHP resource name */


//previously (php5) used MACROS
#define ZEND_FETCH_RESOURCE(rsrc, rsrc_type, passed_id, default_id, resource_type_name, resource_type)        \
        (rsrc = (rsrc_type) zend_fetch_resource(Z_RES_P(*passed_id), resource_type_name, resource_type))
#define ZEND_REGISTER_RESOURCE(return_value, result, le_result)  ZVAL_RES(return_value,zend_register_resource(result, le_result))



//тут будут декларации функций
PHP_FUNCTION(confirm_libtrie_compiled);
PHP_FUNCTION(my_array_fill);
PHP_FUNCTION(yatrie_new);
PHP_FUNCTION(yatrie_load);
PHP_FUNCTION(yatrie_save);
PHP_FUNCTION(yatrie_free);
PHP_FUNCTION(yatrie_add);
PHP_FUNCTION(yatrie_get_id);
PHP_FUNCTION(node_traverse);

static void php_libtrie_dtor(zend_resource *rsrc TSRMLS_DC);

#ifdef PHP_WIN32
#	define PHP_LIBTRIE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_LIBTRIE_API __attribute__ ((visibility("default")))
#else
#	define PHP_LIBTRIE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif


#if defined(ZTS) && defined(COMPILE_DL_LIBTRIE)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif	/* PHP_LIBTRIE_H */