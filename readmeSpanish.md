
# Tiny ESP32 AMSTRAD CPC
Port del emulador de PC de Tom Walker (AMSTRAD CPC) a la placa TTGO VGA32 v1.4 con ESP32.
<br>
<center><img src='https://raw.githubusercontent.com/rpsubc8/ESP32TinyCPC/main/preview/previewCPC464.gif'></center>
He realizado varias modificaciones:
<ul>
 <li>Portado de x86 PC a ESP32</li>
 <li>Uso de un sólo core</li>
 <li>OSD de bajos recursos</li>
 <li>Creado proyecto compatible con Arduino IDE y Platform IO</li>
 <li>Emulación de AY8912 (libreria fabgl) <strike>versión reducida de <b>dcrespo3d</b>(David Crespo Tascón)</strike> versión 1.0.9</li>
 <li>Ajuste de pantalla X</li>
 <li>Menú de velocidad de CPU de emulación (AUTO), sonido, teclado</li>
 <li>Soporte para modo 8 y 64 colores (versión reducida de Ricardo Massaro).</li>
 <li>Soporte DSK de 42 tracks y 11 sectores.</li>
 <li>Carga de archivos DSK desde la tarjeta SD (deben estar en /dsk).</li>
 <li>Emula sólo el CRTC 0</li>
 <li>Soporta modo 0, 1 y 2 de video</li>
 <li>VGA 400x300</li>
 <li>VGA 320x200 con o sin bordes</li>
 <li>Emulación CPC 464 y 664</li>
 <li>Emulacion CPC 6128 (inestable)</li>
 <li>Emulación de AMX Mouse (librería reducida de <b>Rob Kent</b> jazzycamel)</li>
 <li>Ya se permite tener el modo de video 400x300 64 colores, el modo 128 KB, así como el ratón y el sonido todo activo.</li>
 <li>Paleta monocromo Verde para simular GT65</li>
</ul> 
  
<br>
<h1>Requerimientos</h1>
Se requiere:
 <ul>
  <li>TTGO VGA32 v1.4</li>
  <li>Arduino IDE 1.8.11 Espressif System 2.0.7</li>
  <li>Librería reducida Arduino fabgl 0.9.0 (incluida en proyecto PLATFORMIO)</li>
  <li>Librería reducida Arduino bitluni 0.3.3 (incluida en proyecto)</li>
 </ul>
<center><img src='https://raw.githubusercontent.com/rpsubc8/ESP32TinyCPC/main/preview/ttgovga32v12.jpg'></center> 
<br>
 
 
<h1>Arduino IDE</h1>
Todo el proyecto es compatible con la estructura de Arduino 1.8.11.
Tan sólo tenemos que abrir el <b>CPCem.ino</b> del directorio <b>CPCem</b>.
<center><img src='https://raw.githubusercontent.com/rpsubc8/ESP32TinyCPC/main/preview/previewArduinoIDEpreferences.gif'></center>
Debemos instalar las extensiones de spressif en el gestor de urls adicionales de tarjetas <b>https://dl.espressif.com/dl/package_esp32_index.json</b>
<br>
Ya está preparado el proyecto, de forma que no se necesita ninguna librería de bitluni ni fabgl.
Debemos activar la opción de PSRAM, y en caso de superar 1 MB de binario, seleccionar 4 MB de partición a la hora de subir.
En el Arduino IDE, debemos elegir la opción <b>Partition Scheme (Huge APP)</b>.


<br>
<h1>Usabilidad</h1>
Se permiten las siguientes acciones desde el menú (tecla F1):
 <ul>
  <li>Seleccionar Máquina permite elegir CPC 464, 664 o 6128.</li>  
  <li>Seleccionar DSK permite elegir el disco.</li>
  <li>Offset X de la pantalla</li>
  <li>Permite saltar un frame</li>
  <li>Cambiar los milisegundos de polling para video, teclado, ratón y sonido</li>
  <li>Cambiar los milisegundos de espera en cada frame</li>
  <li>Sonido Volumen (100%, 75,%, 50%, 25%, 5%)</li>
  <li>Sonido activo o en silencio</li>
  <li>Espera de CPU en modo AUTO (ajuste 20 ms por frame real) o la espera en ms que queramos</li>
  <li>Ratón Detectar, permite volver a inicializar el ratón, si se ha desconectado en caliente</li>
  <li>Ratón ON y OFF, permite inhabilitar la lectura del ratón. De está forma la emulación va más rápida.</li>
 </ul>
 Se dispone de un OSD básico de bajos recursos, es decir, muy simple, que se visualiza pulsando la tecla <b>F1</b>.
 <center><img src='https://raw.githubusercontent.com/rpsubc8/ESP32TinyCPC/main/preview/previewOSD.gif'></center>
 
 
<br>
<h1>Opciones</h1>
El archivo <b>gbConfig.h</b> se seleccionan las opciones:
<ul>
 <li><b>use_lib_400x300:</b> Se usa modo de vídeo 400x300.</li> 
 <li><b>use_lib_320x200_video_border:</b> Se usa modo de vídeo 320x200 con bordes, de manera que se reduce el tamaño de la pantalla a la mitad. Este modo consume menos RAM que el 400x300 y es más rápido. El modo con bordes, es más lento que el normal, dado que realiza el ajuste de aspecto.</li> 
 <li><b>use_lib_320x200_video_noborder:</b> Se usa modo de vídeo 320x200 sin bordes. En este modo no se reduce la pantalla a la mitad, pero se pierden los bordes. Este modo consume menos RAM que el 400x300 y es más rápido.</li>
 <li><b>use_lib_vga8colors:</b> Obliga a usar RGB modo de 8 colores (3 pines). Saca 8 colores, frente a los 64 del modo normal (6 pines RRGGBB).</li>   
 <li><b>use_lib_sound_ay8912:</b> Se utiliza un mezclador de 3 canales en modo dirty, emulando el AY8912. Consume un poco de RAM. Se requiere la librería fabgl 0.9.0 reducida, ya incluida en el proyecto</li>    
 <li><b>use_lib_log_serial:</b> Se envian logs por puerto serie usb</li>
 <li><b>usb_lib_optimice_checkchange_bankswitch:</b> Sólo conmuta bancos cuando son distintos, ganando velocidad.</li>
 <li><b>use_lib_128k:</b> Permite usar el modo 128K, incluyendo la rom del 6128 en la compilación, así como 2 bloques de memoria de 64KB. Está en fase de pruebas, y al requerir más RAM, se tiene que usar otras opciones de configuración de bajo consumo de RAM.</li>
 <li><b>use_lib_cheat_128k:</b> Modo experimental de 128 KB.</li> 
 <li><b>gb_ms_keyboard:</b> Se debe especificar el número de milisegundos de polling para el teclado.</li>
 <li><b>gb_ms_sound:</b> Se debe especificar el número de milisegundos de polling para el sonido.</li>
 <li><b>gb_frame_crt_skip:</b> Si es 1 se salta un frame.</li>
 <li><b>gb_delay_emulate_ms:</b> Milisegundos de espera por cada frame completado.</li>
 <li><b>use_lib_amx_mouse:</b> Se usa un ratón PS/2 como si fuera un AMX mouse (emulado).</li> El uso del ratón consume un poco de CPU y memoria.
 <li><b>gb_delay_init_ms:</b> Especifica un número de milisegundos inciales de espera en el arranque del ratón, para que lo detecte correctamente.
 <li><b>use_lib_amx_mouse_lefthanded:</b> Ratón para zurdos (intercambia botones)</li> 
 <li><b>gb_ms_mouse:</b> Muestreo en milisegundos de cada lectura de ratón.</li>
 <li><b>use_lib_lookup_znptable:</b> Usa tabla precalculada en FLASH para cálculos BCD. Consume 256 bytes.</li>
 <li><b>use_lib_lookup_znptable16:</b> Usa tabla precalculada en FLASH para cálculos BCD. Consume 64 KBs.</li>
</ul>



<br>
<h1>AMX Mouse</h1>
Para poder usar un ratón como si fuera un AMX Mouse, se requiere activar en el fichero de configuración el soporte.
<center><img src='https://raw.githubusercontent.com/rpsubc8/ESP32TinyCPC/main/preview/previewAmxMouse.gif'></center>
Se permite configurar el ratón para zurdos (también desde el OSD), así como los tiempos de muestreo, todo ello en el fichero <b>gbConfig.h</b>.

<br><br>
<h1>Cargar ROMS</h1>
Los juegos en formato ROM (16 KB), se pueden cargar en LOW y en HIGH (slot 0 al 15). Cuando se selecciona la opción de <b>Run ROM</b> se autoescribe el nombre de la ROM que se invoca como comando RSX. Este comando RSX coincide con el nombre del archivo rom que se generó, por lo que debemos darle el nombre de archivo .ROM exacto al que se invoca.
<br>
Cuando queramos cargar otro juego, debemos resetear, o bien seleccionando el menú de Machine o bien Reset.
<br>
Si queremos lanzar el juego <b>arkanoid</b>, debemos elegir desde el menú <b>Load or Run ROM</b>, luego <b>Run ROM</b> y por último nos pedirá seleccionar <b>Arkanoid</b>. Al finalizar, seleccionamos <b>High ROM(0..15)</b>, en donde ponemos por ejemplo el slot 1, y en un par de segundos, se escribirá <b>|arkanoid</b>.
<br>
Si elegimos sólo <b>Load ROM</b>, hará lo mismo que lo anterior, pero sin lanzar el RSX <b>|arkanoid</b>. Este nombre de archivo debe coincidir con el nombre real del juego interno.
<br>
Si tenemos juego de varias ROMS, por ejemplo <b>3weeks.rom y 3weeks2.rom</b>, debemos cargar 3weeks.rom en slot 1 y 3weeks2.rom en slot 2, y por último lanzar el principal, que en este caso sería <b>3weeks.rom</b>
<br><br>
Más información de ROMS:
<a href='https://www.cpcwiki.eu/index.php/ROM_List'>https://www.cpcwiki.eu/index.php/ROM_List</a>
 

<br><br>
<h1>Monocromo</h1>
En el modo de 64 colores, se ha añadido la posibilidad de elegir 3 tipos de paleta de color monocromo verde. Dado que el DAC es de 6 bits, sólo se permite 11 niveles de verde, además del negro.
<center><img src='https://raw.githubusercontent.com/rpsubc8/ESP32TinyCPC/main/preview/paletaVerde.gif'></center>
Por tanto, no podemos conseguir la misma calidad que un monitor GT65.
<center><img src='https://raw.githubusercontent.com/rpsubc8/ESP32TinyCPC/main/preview/wecLemans.gif'></center>



<br><br>
<h1>CPM</h1>
Para ejecutar el Sistema Operativo CPM se requiere el DSK del mismo (no incluido en este proyecto), y pulsar la tecla |.<br>
<center><img src='https://raw.githubusercontent.com/rpsubc8/ESP32TinyCPC/main/preview/previewCPM.gif'></center>
La Tecla <b>Tilde</b> se saca con la combinación <b>SHIFT</b> + <b>F10</b>.

