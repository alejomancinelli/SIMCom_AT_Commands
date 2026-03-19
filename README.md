# SIMCom_AT_Commands

Librería de para la comunicación mediantes comandos AT con los módulos SIMCom A7670X.

Para mayor información sobre la comunicación y el conjunto completo de comandos disponibles para el módulo SIMCom A7670X, consultar el manual técnico correspondiente.

## Comunicación

La comunicación con estos integrados se realiza mediante comandos AT a través del protocolo UART. Los comandos AT (Attention Commands) constituyen un conjunto de instrucciones estandarizadas utilizadas para controlar módems y módulos de comunicación. En general, el microcontrolador envía comandos en formato de texto al módulo, el cual responde con códigos de estado o con la información solicitada. Este mecanismo permite una integración flexible del módulo dentro del firmware del dispositivo.

## Archivos

### Includes
#### simcom_config.h
Contiene las estructuras de datos utilizadas para la configuración del dispositivo.

#### simcom_types.h
Contiene las enumeraciones de códigos de respuestas del módulo para distintos comandos utilizados.

#### simcom.h
Es el header file a declarar para hacer uso de la librería. Contiene las declaraciones de todas las funciones disponibles en la librería a las cuales puede acceder el usuario.

### Sources
#### at
Este archivo constituye la base de la librería, ya que contiene las funciones necesarias para la comunicación con el módulo mediante comandos AT. Implementa la lógica principal de transmisión y recepción de datos a través del protocolo UART.

Incluye la tarea ```_s_parser_task_fn```, encargada de la lectura continua de los datos recibidos por UART y de su procesamiento. Esta tarea detecta los finales de línea de las respuestas enviadas por el módulo y construye un buffer con las cadenas completas recibidas, permitiendo su posterior análisis por parte de las funciones de la librería.

Entre las funciones principales se encuentran ```simcom_cmd_sync```, utilizada para enviar un comando AT de manera sincrónica y esperar la respuesta del módulo; ```simcom_wait_resp```, que permite esperar una respuesta específica durante un tiempo determinado; y diversas funciones auxiliares destinadas a interpretar los distintos tipos de respuesta que puede generar el módulo según el comando ejecutado.

Por último, la función ```_response_is_urc``` es utilizada por la tarea de parsing para identificar e ignorar los mensajes URC (Unsolicited Response Codes). Estos mensajes son generados de forma asíncrona por el módulo —por ejemplo, para indicar cambios en el estado de la red o eventos internos— y pueden interferir con la interpretación de las respuestas esperadas a los comandos enviados. Actualmente se incluyen los URC más comunes, aunque se recomienda realizar un análisis más exhaustivo para mejorar la robustez del sistema.

#### module
Este archivo contiene las funciones relacionadas con la configuración del puerto de comunicación UART y la inicialización del módulo SIMCom A7670X. En él se implementan las rutinas necesarias para configurar los parámetros de comunicación serial, así como las secuencias iniciales de verificación y preparación del módulo antes de comenzar a utilizar los distintos servicios disponibles.

#### services
Cada archivo source dentro de esta sección representa un tipo de servicio provisto por el módulo SIMCom A7670X. La organización de estos archivos sigue la estructura definida en el manual de comandos AT del fabricante, separando las funcionalidades según el tipo de servicio que implementan (por ejemplo, red, datos, SMS u otras capacidades del módem).
Todas las funciones implementadas siguen una estructura de operación similar: se envía el comando AT correspondiente, se espera la respuesta del módulo dentro de un tiempo determinado y posteriormente se analiza la respuesta para determinar el resultado de la operación.

Actualmente solo se han implementado las funciones necesarias para el funcionamiento básico del módulo dentro del sistema. No obstante, la estructura de la librería permite extender fácilmente sus capacidades agregando nuevas funciones que implementen comandos adicionales según los requerimientos del proyecto.
