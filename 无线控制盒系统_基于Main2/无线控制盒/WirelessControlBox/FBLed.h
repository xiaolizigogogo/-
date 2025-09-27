#ifndef __FB_LED_H
#define __FB_LED_H
#include <string.h>



class FBLed{
	public:
		void begin(int dio,int rclk,int sclk);
		void ledShow(const char * str);
		void ledout(unsigned char chr);
	private:
		int DIO;
		int RCLK;
		int SCLK;

};
#endif