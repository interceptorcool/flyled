/*
 * ESP8266HTTPFileUpload.cpp
 *
 *  Created on: 22 мар. 2017 г.
 *      Author: icool
 */
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "ESP8266HTTPFileUpload.h"
#include <FS.h>
#include <WString.h>
#include "board_pin.h"


const char* ESP8266HTTPFileUpload::_serverIndex =
R"(<html><body><form method='POST' action='' enctype='multipart/form-data'>
<h1><p>UPLOAD CONFIG file. Example config.json</p></h1>
<input type='file' name='upload' accept=".json">
<input type='submit' value='Upload'>
</form>
</body></html>)";
const char* ESP8266HTTPFileUpload::_failedResponse = R"(Upload Failed!)";
const char* ESP8266HTTPFileUpload::_successResponse = "<META http-equiv=\"refresh\" content=\"15;URL=\">Upload Success!";



ESP8266HTTPFileUpload::ESP8266HTTPFileUpload()
{
  _server = NULL;
  _reboot_after_write = false;
  _file_name = nullptr;
  _upload_success = _upload_begin = false;
  _buffer = nullptr;
  _bufferWrPos = 0;
}


void ESP8266HTTPFileUpload::setup(ESP8266WebServer *server, const char * path, const char *file_name, bool reboot)
{
    _server = server;
    _reboot_after_write = reboot;
    _file_name = file_name;

    // handler for the /update form page
    _server->on(path, HTTP_GET, [&](){
      _server->send(200, "text/html", _serverIndex);
    });

    // handler for the /update form POST (once file upload finishes)
    _server->on(path, HTTP_POST, [&](){
    	String status;
    	if (_upload_success)
    	{
    		status = _successResponse;
    		if (_reboot_after_write)
    			status += "Rebooting...";
    	} else
    		status = _failedResponse;

		_server->send(200, "text/html", status);

		if (_reboot_after_write && _upload_success)
		{
			ESP.restart();
		}
    },[&](){
      // handler for the file upload, get's the sketch bytes, and writes
      // them through the Update object
      HTTPUpload& upload = _server->upload();
      digitalWrite(FLYLED_STATUS_LED_PIN, !digitalRead(FLYLED_STATUS_LED_PIN));
      	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      if (upload.status == UPLOAD_FILE_START) {

    	WiFiUDP::stopAll();

		_upload_begin = (upload.filename.length() && upload.filename.indexOf(F(".json")));
        if (_upload_begin)
        {

			if (_file_name == nullptr)
			{
				_file_name = upload.filename.c_str();
			}

			_buffer = new uint8_t[FLASH_SECTOR_SIZE];
			_bufferWrPos = 0;

        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      } else if (upload.status == UPLOAD_FILE_WRITE && _upload_begin) {
   	  	  // запоминаем принятое
   	  	  if (_bufferWrPos + upload.currentSize < FLASH_SECTOR_SIZE)
   	  	  {
#ifdef DEBUG_UPLOAD
   	  		  Serial.print(".");
#endif
   	  		  memcpy(_buffer + _bufferWrPos, upload.buf, upload.currentSize);
   	  		  _bufferWrPos += upload.currentSize;
   	  	  } else {
   	  		  String n = String("/T") + String(_file_name);
   	  		  File f = SPIFFS.open(n, "a");
   	  		  if (f)
   	  		  {
   	  			  if (_bufferWrPos && f.write(_buffer, _bufferWrPos))
   	  				  DEBUG_UPLOAD("Error write = %d\n", f.getWriteError());
   	  			  if (!_bufferWrPos && upload.currentSize)
   	   	  			  if (f.write(upload.buf, upload.currentSize))
   	   	  				  DEBUG_UPLOAD("Error write = %d\n", f.getWriteError());

   	  			  f.close();
   	  		  }
   	  		  _bufferWrPos = 0;
   	  	  }

      	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      } else if(upload.status == UPLOAD_FILE_END && _upload_begin) {

    	  if (upload.totalSize)
		{
			String n =  String("/T") + String(_file_name);
			File f = SPIFFS.open(n, "a");
			if (f && _bufferWrPos)
			{
				if (f.write(_buffer, _bufferWrPos))
					DEBUG_UPLOAD("Error write = %d\n", f.getWriteError());

				f.close();

				// rename file
				String fileRename =  String("/") + String(_file_name);

				// удаляем существующий файл
				if (SPIFFS.exists(fileRename))
					SPIFFS.remove(fileRename);

				if (!SPIFFS.rename(n, fileRename))
				{
					DEBUG_UPLOAD("Error rename file %s \n", _file_name);
				}

				_upload_success = true;
				DEBUG_UPLOAD("upload success: %s : size: %d\n", fileRename.c_str(), upload.totalSize);

				if (_reboot_after_write)
				{
					DEBUG_UPLOAD("Rebooting...\n");
				}

			}
		} else {
			DEBUG_UPLOAD("Error file %s size upload: web size: %u \n", _file_name, upload.totalSize);
		}

		if (_buffer)
			delete[] _buffer;


      } else if(upload.status == UPLOAD_FILE_ABORTED){

    	if (_buffer)
    		delete[] _buffer;

    	DEBUG_UPLOAD("Upload was aborted\n");
      }
      delay(0);
    });
}

