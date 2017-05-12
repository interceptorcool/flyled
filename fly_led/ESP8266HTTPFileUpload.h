/*
 * ESP8266HTTPFileUpload.h
 *
 *  Created on: 22 мар. 2017 г.
 *      Author: icool
 */

#ifndef FLY_LED_ESP8266HTTPFILEUPLOAD_H_
#define FLY_LED_ESP8266HTTPFILEUPLOAD_H_

#include <FS.h>
#include <string.h>
#include <stdint.h>

//#define DEBUG_UPLOAD
#define DEBUG_UPLOAD_PORT Serial

#ifdef DEBUG_UPLOAD
#ifdef DEBUG_UPLOAD_PORT
#define DEBUG_UPLOAD(...) DEBUG_UPLOAD_PORT.printf("UP: "); DEBUG_UPLOAD_PORT.printf( __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_UPLOAD
#define DEBUG_UPLOAD(...)
#endif



class ESP8266WebServer;

class ESP8266HTTPFileUpload
{
private:
  bool _reboot_after_write;
  //File _uploadFile;
  const char *_file_name;
  bool _upload_success;
  bool _upload_begin;

  uint8_t	*_buffer;
  size_t	_bufferWrPos;


  ESP8266WebServer *_server;

  static const char *_serverIndex;
  static const char *_failedResponse;
  static const char *_successResponse;

public:
	ESP8266HTTPFileUpload();

    void setup(ESP8266WebServer *server)
    {
      setup(server, nullptr, false);
    }

    void setup(ESP8266WebServer *server, bool reboot)
    {
      setup(server, nullptr, reboot);
    }

    void setup(ESP8266WebServer *server, const char *file_name, bool reboot)
    {
    	setup(server, "/upload", file_name, reboot);
    }

    void setup(ESP8266WebServer *server, const char * path, const char *file_name, bool reboot);

};

#endif /* FLY_LED_ESP8266HTTPFILEUPLOAD_H_ */
