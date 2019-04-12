#ifndef __YT_LED_H__
#define __YT_LED_H__

enum LED_SCENE
{
    LED_IDLE,
    LED_CONNECTING_WIFI,
    LED_PLAY,
    LED_SEND_CONTENT,
    LED_SENDING,
    LED_SHUTDOWN,
};

class YTLED
{
public:
	static YTLED& instance();
	
	YTLED::YTLED();

	~YTLED();

	void open_power();
	void close_power();


	void disp_head_led();
	void close_head_led();
	
	void click_led_button();
	void disp_led(LED_SCENE scene);
private:

	static YTLED _s_instance;

};
#endif
